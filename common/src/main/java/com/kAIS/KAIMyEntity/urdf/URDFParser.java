package com.kAIS.KAIMyEntity.urdf;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joml.Vector3f;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import java.io.File;

public class URDFParser {
    private static final Logger logger = LogManager.getLogger();
    private static File baseDir; // URDF 파일이 있는 디렉토리
    
    public static URDFRobotModel parse(File urdfFile) {
        try {
            baseDir = urdfFile.getParentFile();
            logger.info("=== URDF Parsing Start ===");
            logger.info("File: " + urdfFile.getAbsolutePath());
            logger.info("Base directory: " + baseDir.getAbsolutePath());
            
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document doc = builder.parse(urdfFile);
            doc.getDocumentElement().normalize();
            
            Element robotElement = doc.getDocumentElement();
            String robotName = robotElement.getAttribute("name");
            
            URDFRobotModel robot = new URDFRobotModel(robotName);
            
            // Parse all links
            NodeList linkNodes = robotElement.getElementsByTagName("link");
            logger.info("Found " + linkNodes.getLength() + " links");
            for (int i = 0; i < linkNodes.getLength(); i++) {
                Node node = linkNodes.item(i);
                if (node.getNodeType() == Node.ELEMENT_NODE) {
                    URDFLink link = parseLink((Element) node);
                    if (link != null) {
                        robot.addLink(link);
                        logger.debug("  + Link: " + link.name);
                    }
                }
            }
            
            // Parse all joints
            NodeList jointNodes = robotElement.getElementsByTagName("joint");
            logger.info("Found " + jointNodes.getLength() + " joints");
            for (int i = 0; i < jointNodes.getLength(); i++) {
                Node node = jointNodes.item(i);
                if (node.getNodeType() == Node.ELEMENT_NODE) {
                    URDFJoint joint = parseJoint((Element) node);
                    if (joint != null) {
                        robot.addJoint(joint);
                        logger.debug("  + Joint: " + joint.name + " (" + joint.type + ")");
                    }
                }
            }
            
            robot.buildHierarchy();
            
            // ✓ 루트 링크 검증
            if (robot.rootLinkName == null || robot.getLink(robot.rootLinkName) == null) {
                logger.error("✗ No valid root link found!");
                logger.error("  Check parent/child relationships in joints");
                return null;
            }
            
            logger.info("=== URDF Parsing Complete ===");
            logger.info("  Robot: " + robot.name);
            logger.info("  Links: " + robot.getLinkCount());
            logger.info("  Joints: " + robot.getJointCount());
            logger.info("  Movable Joints: " + robot.getMovableJointCount());
            logger.info("  Root Link: " + robot.rootLinkName);
            
            return robot;
            
        } catch (Exception e) {
            logger.error("✗ Failed to parse URDF file: " + urdfFile.getAbsolutePath(), e);
            return null;
        }
    }
    
    // ========== 메시 경로 해석 (개선) ==========
    
    private static String resolveMeshPath(String uri) {
        if (uri == null || uri.isEmpty()) {
            return null;
        }
        
        logger.debug("Resolving mesh URI: " + uri);
        
        // 1. file:// 스킴
        if (uri.startsWith("file://")) {
            try {
                String path = new java.net.URI(uri).getPath();
                File f = new File(path);
                if (f.exists()) {
                    logger.debug("  -> file:// resolved: " + f.getAbsolutePath());
                    return f.getAbsolutePath();
                } else {
                    logger.warn("  -> file:// path not found: " + path);
                    return null;
                }
            } catch (Exception e) {
                logger.warn("Invalid file:// URI: " + uri);
                return null;
            }
        }
        
        // 2. package:// 스킴 (ROS 표준)
        if (uri.startsWith("package://")) {
            String withoutScheme = uri.substring("package://".length());
            int slash = withoutScheme.indexOf('/');
            String relativePath = (slash >= 0) ? withoutScheme.substring(slash + 1) : withoutScheme;
            
            File resolved = new File(baseDir, relativePath);
            if (resolved.exists()) {
                logger.debug("  -> package:// resolved: " + resolved.getAbsolutePath());
                return resolved.getAbsolutePath();
            } else {
                logger.warn("  -> package:// not found: " + resolved.getAbsolutePath());
                // Fallback: meshes/ 폴더에서 찾기
                File meshDir = new File(baseDir, "meshes");
                File fallback = new File(meshDir, new File(relativePath).getName());
                if (fallback.exists()) {
                    logger.info("  -> Found in meshes/: " + fallback.getAbsolutePath());
                    return fallback.getAbsolutePath();
                }
                return null;
            }
        }
        
        // 3. 절대 경로
        File f = new File(uri);
        if (f.isAbsolute()) {
            if (f.exists()) {
                logger.debug("  -> absolute path: " + f.getAbsolutePath());
                return f.getAbsolutePath();
            } else {
                logger.warn("  -> absolute path not found: " + uri);
                return null;
            }
        }
        
        // 4. 상대 경로 (baseDir 기준)
        File resolved = new File(baseDir, uri);
        if (resolved.exists()) {
            logger.debug("  -> relative to baseDir: " + resolved.getAbsolutePath());
            return resolved.getAbsolutePath();
        }
        
        // 5. Fallback: meshes/ 폴더에서 파일명만으로 찾기
        String filename = new File(uri).getName();
        File meshDir = new File(baseDir, "meshes");
        
        // meshes/ 폴더 자체가 없으면 생성하지 않고 경고만
        if (meshDir.exists() && meshDir.isDirectory()) {
            File meshFile = new File(meshDir, filename);
            if (meshFile.exists()) {
                logger.info("  -> Found by filename in meshes/: " + meshFile.getAbsolutePath());
                return meshFile.getAbsolutePath();
            }
            
            // 대소문자 무시하고 찾기 (Windows 호환성)
            File[] meshFiles = meshDir.listFiles();
            if (meshFiles != null) {
                for (File candidate : meshFiles) {
                    if (candidate.getName().equalsIgnoreCase(filename)) {
                        logger.info("  -> Found by case-insensitive search: " + candidate.getAbsolutePath());
                        return candidate.getAbsolutePath();
                    }
                }
            }
        }
        
        // 6. 최종 Fallback: 하위 디렉토리 재귀 검색
        File found = searchFileRecursive(baseDir, filename, 2);
        if (found != null) {
            logger.info("  -> Found by recursive search: " + found.getAbsolutePath());
            return found.getAbsolutePath();
        }
        
        logger.warn("  -> Could not resolve mesh: " + uri);
        return null;
    }
    
    /**
     * 파일을 재귀적으로 검색 (최대 깊이 제한)
     */
    private static File searchFileRecursive(File directory, String filename, int maxDepth) {
        if (maxDepth <= 0 || !directory.isDirectory()) {
            return null;
        }
        
        File[] files = directory.listFiles();
        if (files == null) {
            return null;
        }
        
        // 현재 디렉토리에서 검색
        for (File file : files) {
            if (file.isFile() && file.getName().equalsIgnoreCase(filename)) {
                return file;
            }
        }
        
        // 하위 디렉토리 재귀 검색
        for (File file : files) {
            if (file.isDirectory() && !file.getName().startsWith(".")) {
                File found = searchFileRecursive(file, filename, maxDepth - 1);
                if (found != null) {
                    return found;
                }
            }
        }
        
        return null;
    }
    
    // ========== 파싱 메서드들 ==========
    
    private static URDFLink parseLink(Element linkElement) {
        String name = linkElement.getAttribute("name");
        URDFLink link = new URDFLink(name);
        
        // Parse visual
        NodeList visualNodes = linkElement.getElementsByTagName("visual");
        if (visualNodes.getLength() > 0) {
            link.visual = parseVisual((Element) visualNodes.item(0));
        }
        
        // Parse collision
        NodeList collisionNodes = linkElement.getElementsByTagName("collision");
        if (collisionNodes.getLength() > 0) {
            link.collision = parseCollision((Element) collisionNodes.item(0));
        }
        
        // Parse inertial
        NodeList inertialNodes = linkElement.getElementsByTagName("inertial");
        if (inertialNodes.getLength() > 0) {
            link.inertial = parseInertial((Element) inertialNodes.item(0));
        }
        
        return link;
    }
    
    private static URDFLink.Visual parseVisual(Element visualElement) {
        URDFLink.Visual visual = new URDFLink.Visual();
        
        // Parse origin
        NodeList originNodes = visualElement.getElementsByTagName("origin");
        if (originNodes.getLength() > 0) {
            visual.origin = parseOrigin((Element) originNodes.item(0));
        }
        
        // Parse geometry
        NodeList geometryNodes = visualElement.getElementsByTagName("geometry");
        if (geometryNodes.getLength() > 0) {
            visual.geometry = parseGeometry((Element) geometryNodes.item(0));
        }
        
        // Parse material
        NodeList materialNodes = visualElement.getElementsByTagName("material");
        if (materialNodes.getLength() > 0) {
            visual.material = parseMaterial((Element) materialNodes.item(0));
        }
        
        return visual;
    }
    
    private static URDFLink.Collision parseCollision(Element collisionElement) {
        URDFLink.Collision collision = new URDFLink.Collision();
        
        NodeList originNodes = collisionElement.getElementsByTagName("origin");
        if (originNodes.getLength() > 0) {
            collision.origin = parseOrigin((Element) originNodes.item(0));
        }
        
        NodeList geometryNodes = collisionElement.getElementsByTagName("geometry");
        if (geometryNodes.getLength() > 0) {
            collision.geometry = parseGeometry((Element) geometryNodes.item(0));
        }
        
        return collision;
    }
    
    private static URDFLink.Inertial parseInertial(Element inertialElement) {
        URDFLink.Inertial inertial = new URDFLink.Inertial();
        
        NodeList originNodes = inertialElement.getElementsByTagName("origin");
        if (originNodes.getLength() > 0) {
            inertial.origin = parseOrigin((Element) originNodes.item(0));
        }
        
        NodeList massNodes = inertialElement.getElementsByTagName("mass");
        if (massNodes.getLength() > 0) {
            inertial.mass = new URDFLink.Inertial.Mass();
            inertial.mass.value = Float.parseFloat(((Element) massNodes.item(0)).getAttribute("value"));
        }
        
        NodeList inertiaNodes = inertialElement.getElementsByTagName("inertia");
        if (inertiaNodes.getLength() > 0) {
            Element inertiaEl = (Element) inertiaNodes.item(0);
            inertial.inertia = new URDFLink.Inertial.Inertia();
            inertial.inertia.ixx = Float.parseFloat(inertiaEl.getAttribute("ixx"));
            inertial.inertia.ixy = Float.parseFloat(inertiaEl.getAttribute("ixy"));
            inertial.inertia.ixz = Float.parseFloat(inertiaEl.getAttribute("ixz"));
            inertial.inertia.iyy = Float.parseFloat(inertiaEl.getAttribute("iyy"));
            inertial.inertia.iyz = Float.parseFloat(inertiaEl.getAttribute("iyz"));
            inertial.inertia.izz = Float.parseFloat(inertiaEl.getAttribute("izz"));
        }
        
        return inertial;
    }
    
    private static URDFLink.Geometry parseGeometry(Element geometryElement) {
        URDFLink.Geometry geometry = new URDFLink.Geometry();
        
        // Check for mesh
        NodeList meshNodes = geometryElement.getElementsByTagName("mesh");
        if (meshNodes.getLength() > 0) {
            Element meshEl = (Element) meshNodes.item(0);
            geometry.type = URDFLink.Geometry.GeometryType.MESH;
            
            String rawUri = meshEl.getAttribute("filename");
            String resolved = resolveMeshPath(rawUri);
            
            if (resolved != null) {
                geometry.meshFilename = resolved;
                logger.debug("✓ Mesh resolved: " + new File(resolved).getName());
            } else {
                logger.warn("✗ Failed to resolve mesh: " + rawUri);
                geometry.meshFilename = rawUri; // 원본 저장
            }
            
            // Parse scale
            if (meshEl.hasAttribute("scale")) {
                geometry.scale = parseVector3(meshEl.getAttribute("scale"));
            } else {
                geometry.scale = new Vector3f(1f, 1f, 1f);
            }
            
            return geometry;
        }
        
        // Check for box
        NodeList boxNodes = geometryElement.getElementsByTagName("box");
        if (boxNodes.getLength() > 0) {
            Element boxEl = (Element) boxNodes.item(0);
            geometry.type = URDFLink.Geometry.GeometryType.BOX;
            geometry.boxSize = parseVector3(boxEl.getAttribute("size"));
            return geometry;
        }
        
        // Check for cylinder
        NodeList cylinderNodes = geometryElement.getElementsByTagName("cylinder");
        if (cylinderNodes.getLength() > 0) {
            Element cylEl = (Element) cylinderNodes.item(0);
            geometry.type = URDFLink.Geometry.GeometryType.CYLINDER;
            geometry.cylinderRadius = Float.parseFloat(cylEl.getAttribute("radius"));
            geometry.cylinderLength = Float.parseFloat(cylEl.getAttribute("length"));
            return geometry;
        }
        
        // Check for sphere
        NodeList sphereNodes = geometryElement.getElementsByTagName("sphere");
        if (sphereNodes.getLength() > 0) {
            Element sphereEl = (Element) sphereNodes.item(0);
            geometry.type = URDFLink.Geometry.GeometryType.SPHERE;
            geometry.sphereRadius = Float.parseFloat(sphereEl.getAttribute("radius"));
            return geometry;
        }
        
        logger.warn("No geometry found in element");
        return geometry;
    }
    
    private static URDFLink.Material parseMaterial(Element materialElement) {
        URDFLink.Material material = new URDFLink.Material();
        material.name = materialElement.getAttribute("name");
        
        // Parse color
        NodeList colorNodes = materialElement.getElementsByTagName("color");
        if (colorNodes.getLength() > 0) {
            Element colorEl = (Element) colorNodes.item(0);
            String rgbaStr = colorEl.getAttribute("rgba");
            String[] rgba = rgbaStr.trim().split("\\s+");
            if (rgba.length == 4) {
                material.color = new URDFLink.Material.Vector4f(
                    Float.parseFloat(rgba[0]),
                    Float.parseFloat(rgba[1]),
                    Float.parseFloat(rgba[2]),
                    Float.parseFloat(rgba[3])
                );
            }
        }
        
        // Parse texture
        NodeList textureNodes = materialElement.getElementsByTagName("texture");
        if (textureNodes.getLength() > 0) {
            Element textureEl = (Element) textureNodes.item(0);
            material.textureFilename = textureEl.getAttribute("filename");
        }
        
        return material;
    }
    
    private static URDFLink.Origin parseOrigin(Element originElement) {
        URDFLink.Origin origin = new URDFLink.Origin();
        
        if (originElement.hasAttribute("xyz")) {
            origin.xyz = parseVector3(originElement.getAttribute("xyz"));
        }
        
        if (originElement.hasAttribute("rpy")) {
            origin.rpy = parseVector3(originElement.getAttribute("rpy"));
        }
        
        return origin;
    }
    
    private static URDFJoint parseJoint(Element jointElement) {
        String name = jointElement.getAttribute("name");
        String typeStr = jointElement.getAttribute("type").toUpperCase();
        
        URDFJoint.JointType type;
        try {
            type = URDFJoint.JointType.valueOf(typeStr);
        } catch (IllegalArgumentException e) {
            logger.warn("Unknown joint type: " + typeStr + ", defaulting to FIXED");
            type = URDFJoint.JointType.FIXED;
        }
        
        URDFJoint joint = new URDFJoint(name, type);
        
        // Parse parent link
        NodeList parentNodes = jointElement.getElementsByTagName("parent");
        if (parentNodes.getLength() > 0) {
            joint.parentLinkName = ((Element) parentNodes.item(0)).getAttribute("link");
        }
        
        // Parse child link
        NodeList childNodes = jointElement.getElementsByTagName("child");
        if (childNodes.getLength() > 0) {
            joint.childLinkName = ((Element) childNodes.item(0)).getAttribute("link");
        }
        
        // Parse origin
        NodeList originNodes = jointElement.getElementsByTagName("origin");
        if (originNodes.getLength() > 0) {
            if (joint.origin == null) joint.origin = new URDFJoint.Origin();
            Element originEl = (Element) originNodes.item(0);
            if (originEl.hasAttribute("xyz")) {
                joint.origin.xyz = parseVector3(originEl.getAttribute("xyz"));
            }
            if (originEl.hasAttribute("rpy")) {
                joint.origin.rpy = parseVector3(originEl.getAttribute("rpy"));
            }
        }
        
        // ✓ Parse axis with URDF defaults
        NodeList axisNodes = jointElement.getElementsByTagName("axis");
        Vector3f axis = null;
        
        if (axisNodes.getLength() > 0) {
            Element axisEl = (Element) axisNodes.item(0);
            axis = parseVector3(axisEl.getAttribute("xyz"));
        } else {
            // URDF 표준: axis 없으면 타입별 기본값
            switch (type) {
                case REVOLUTE:
                case CONTINUOUS:
                case PRISMATIC:
                    axis = new Vector3f(1, 0, 0); // X축
                    logger.debug("Joint '" + name + "': using default axis (1,0,0)");
                    break;
                default:
                    axis = new Vector3f(0, 0, 0); // FIXED 등
            }
        }
        
        if (joint.axis == null) joint.axis = new URDFJoint.Axis();
        joint.axis.xyz = axis;
        
        // ✓ 0벡터 정규화 방지
        if (axis.lengthSquared() > 1e-12f) {
            axis.normalize();
        } else {
            if (type == URDFJoint.JointType.REVOLUTE || 
                type == URDFJoint.JointType.CONTINUOUS || 
                type == URDFJoint.JointType.PRISMATIC) {
                logger.warn("Joint '" + name + "' has zero axis; using (1,0,0) fallback");
                joint.axis.xyz.set(1, 0, 0);
            }
        }
        
        // Parse limit
        NodeList limitNodes = jointElement.getElementsByTagName("limit");
        if (limitNodes.getLength() > 0) {
            Element limitEl = (Element) limitNodes.item(0);
            joint.limit = new URDFJoint.Limit();
            
            if (limitEl.hasAttribute("lower")) {
                joint.limit.lower = Float.parseFloat(limitEl.getAttribute("lower"));
            }
            if (limitEl.hasAttribute("upper")) {
                joint.limit.upper = Float.parseFloat(limitEl.getAttribute("upper"));
            }
            if (limitEl.hasAttribute("effort")) {
                joint.limit.effort = Float.parseFloat(limitEl.getAttribute("effort"));
            }
            if (limitEl.hasAttribute("velocity")) {
                joint.limit.velocity = Float.parseFloat(limitEl.getAttribute("velocity"));
            }
        }
        
        // Parse dynamics
        NodeList dynamicsNodes = jointElement.getElementsByTagName("dynamics");
        if (dynamicsNodes.getLength() > 0) {
            Element dynamicsEl = (Element) dynamicsNodes.item(0);
            joint.dynamics = new URDFJoint.Dynamics();
            
            if (dynamicsEl.hasAttribute("damping")) {
                joint.dynamics.damping = Float.parseFloat(dynamicsEl.getAttribute("damping"));
            }
            if (dynamicsEl.hasAttribute("friction")) {
                joint.dynamics.friction = Float.parseFloat(dynamicsEl.getAttribute("friction"));
            }
        }
        
        return joint;
    }
    
    private static Vector3f parseVector3(String str) {
        String[] parts = str.trim().split("\\s+");
        if (parts.length == 3) {
            return new Vector3f(
                Float.parseFloat(parts[0]),
                Float.parseFloat(parts[1]),
                Float.parseFloat(parts[2])
            );
        }
        logger.warn("Invalid Vector3 string: " + str);
        return new Vector3f(0.0f, 0.0f, 0.0f);
    }
}
