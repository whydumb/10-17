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
    
    public static URDFRobotModel parse(File urdfFile) {
        try {
            logger.info("Parsing URDF file: " + urdfFile.getAbsolutePath());
            
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document doc = builder.parse(urdfFile);
            doc.getDocumentElement().normalize();
            
            Element robotElement = doc.getDocumentElement();
            String robotName = robotElement.getAttribute("name");
            
            URDFRobotModel robot = new URDFRobotModel(robotName);
            
            // Parse all links
            NodeList linkNodes = robotElement.getElementsByTagName("link");
            for (int i = 0; i < linkNodes.getLength(); i++) {
                Node node = linkNodes.item(i);
                if (node.getNodeType() == Node.ELEMENT_NODE) {
                    URDFLink link = parseLink((Element) node);
                    if (link != null) {
                        robot.addLink(link);
                    }
                }
            }
            
            // Parse all joints
            NodeList jointNodes = robotElement.getElementsByTagName("joint");
            for (int i = 0; i < jointNodes.getLength(); i++) {
                Node node = jointNodes.item(i);
                if (node.getNodeType() == Node.ELEMENT_NODE) {
                    URDFJoint joint = parseJoint((Element) node);
                    if (joint != null) {
                        robot.addJoint(joint);
                    }
                }
            }
            
            robot.buildHierarchy();
            
            logger.info("URDF parsing complete: " + robot.toString());
            return robot;
            
        } catch (Exception e) {
            logger.error("Failed to parse URDF file: " + urdfFile.getAbsolutePath(), e);
            return null;
        }
    }
    
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
        
        // Parse origin
        NodeList originNodes = collisionElement.getElementsByTagName("origin");
        if (originNodes.getLength() > 0) {
            collision.origin = parseOrigin((Element) originNodes.item(0));
        }
        
        // Parse geometry
        NodeList geometryNodes = collisionElement.getElementsByTagName("geometry");
        if (geometryNodes.getLength() > 0) {
            collision.geometry = parseGeometry((Element) geometryNodes.item(0));
        }
        
        return collision;
    }
    
    private static URDFLink.Inertial parseInertial(Element inertialElement) {
        URDFLink.Inertial inertial = new URDFLink.Inertial();
        
        // Parse origin
        NodeList originNodes = inertialElement.getElementsByTagName("origin");
        if (originNodes.getLength() > 0) {
            inertial.origin = parseOrigin((Element) originNodes.item(0));
        }
        
        // Parse mass
        NodeList massNodes = inertialElement.getElementsByTagName("mass");
        if (massNodes.getLength() > 0) {
            inertial.mass = new URDFLink.Inertial.Mass();
            inertial.mass.value = Float.parseFloat(((Element) massNodes.item(0)).getAttribute("value"));
        }
        
        // Parse inertia
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
            geometry.meshFilename = meshEl.getAttribute("filename");
            
            // Parse scale if present
            if (meshEl.hasAttribute("scale")) {
                String scaleStr = meshEl.getAttribute("scale");
                geometry.scale = parseVector3(scaleStr);
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
            Element originEl = (Element) originNodes.item(0);
            if (originEl.hasAttribute("xyz")) {
                joint.origin.xyz = parseVector3(originEl.getAttribute("xyz"));
            }
            if (originEl.hasAttribute("rpy")) {
                joint.origin.rpy = parseVector3(originEl.getAttribute("rpy"));
            }
        }
        
        // Parse axis
        NodeList axisNodes = jointElement.getElementsByTagName("axis");
        if (axisNodes.getLength() > 0) {
            Element axisEl = (Element) axisNodes.item(0);
            joint.axis.xyz = parseVector3(axisEl.getAttribute("xyz"));
            joint.axis.normalize();
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
        return new Vector3f(0.0f, 0.0f, 0.0f);
    }
}