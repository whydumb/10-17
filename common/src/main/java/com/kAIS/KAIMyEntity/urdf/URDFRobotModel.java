package com.kAIS.KAIMyEntity.urdf;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class URDFRobotModel {
    public String name;
    public List<URDFLink> links;
    public List<URDFJoint> joints;
    
    // Quick lookup maps
    private Map<String, URDFLink> linkMap;
    private Map<String, URDFJoint> jointMap;
    private Map<String, List<URDFJoint>> childJointsMap; // parent link -> child joints
    
    public String rootLinkName;
    
    public URDFRobotModel(String name) {
        this.name = name;
        this.links = new ArrayList<>();
        this.joints = new ArrayList<>();
        this.linkMap = new HashMap<>();
        this.jointMap = new HashMap<>();
        this.childJointsMap = new HashMap<>();
    }
    
    public void addLink(URDFLink link) {
        links.add(link);
        linkMap.put(link.name, link);
    }
    
    public void addJoint(URDFJoint joint) {
        joints.add(joint);
        jointMap.put(joint.name, joint);
        
        // Build child joints map for hierarchy
        if (!childJointsMap.containsKey(joint.parentLinkName)) {
            childJointsMap.put(joint.parentLinkName, new ArrayList<>());
        }
        childJointsMap.get(joint.parentLinkName).add(joint);
    }
    
    public URDFLink getLink(String name) {
        return linkMap.get(name);
    }
    
    public URDFJoint getJoint(String name) {
        return jointMap.get(name);
    }
    
    public List<URDFJoint> getChildJoints(String parentLinkName) {
        return childJointsMap.getOrDefault(parentLinkName, new ArrayList<>());
    }
    
    public void buildHierarchy() {
        // Find root link (link with no parent)
        Map<String, Boolean> isChild = new HashMap<>();
        for (URDFJoint joint : joints) {
            isChild.put(joint.childLinkName, true);
        }
        
        for (URDFLink link : links) {
            if (!isChild.containsKey(link.name)) {
                rootLinkName = link.name;
                break;
            }
        }
        
        if (rootLinkName == null && !links.isEmpty()) {
            // Fallback: use first link
            rootLinkName = links.get(0).name;
        }
    }
    
    public void updateJointPosition(String jointName, float position) {
        URDFJoint joint = getJoint(jointName);
        if (joint != null) {
            joint.updatePosition(position);
        }
    }
    
    public void updateJointPositions(Map<String, Float> positions) {
        for (Map.Entry<String, Float> entry : positions.entrySet()) {
            updateJointPosition(entry.getKey(), entry.getValue());
        }
    }
    
    public int getLinkCount() {
        return links.size();
    }
    
    public int getJointCount() {
        return joints.size();
    }
    
    public int getMovableJointCount() {
        int count = 0;
        for (URDFJoint joint : joints) {
            if (joint.isMovable()) {
                count++;
            }
        }
        return count;
    }
    
    @Override
    public String toString() {
        return String.format("URDFRobotModel[name=%s, links=%d, joints=%d, movable=%d, root=%s]",
            name, getLinkCount(), getJointCount(), getMovableJointCount(), rootLinkName);
    }
}
