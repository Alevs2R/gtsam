%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% GTSAM Copyright 2010, Georgia Tech Research Corporation,
% Atlanta, Georgia 30332-0415
% All Rights Reserved
% Authors: Frank Dellaert, et al. (see THANKS for the full author list)
%
% See LICENSE for the license information
%
% @brief A simple visual SLAM example for structure from motion
% @author Duy-Nguyen Ta
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if 0
    %% Create a triangle target, just 3 points on a plane
    nPoints = 3;
    r = 10;
    points = {};
    for j=1:nPoints
        theta = (j-1)*2*pi/nPoints;
        points{j} = gtsamPoint3([r*cos(theta), r*sin(theta), 0]');
    end
else
    %% Generate simulated data
    % 3D landmarks as vertices of a cube
    nPoints = 8;
    points = {gtsamPoint3([10 10 10]'),...
        gtsamPoint3([-10 10 10]'),...
        gtsamPoint3([-10 -10 10]'),...
        gtsamPoint3([10 -10 10]'),...
        gtsamPoint3([10 10 -10]'),...
        gtsamPoint3([-10 10 -10]'),...
        gtsamPoint3([-10 -10 -10]'),...
        gtsamPoint3([10 -10 -10]')};
end

%% Create camera cameras on a circle around the triangle
nCameras = 10;
height = 0;
r = 30;
cameras = {};
K = gtsamCal3_S2(500,500,0,640/2,480/2);
for i=1:nCameras
    theta = (i-1)*2*pi/nCameras;
    t = gtsamPoint3([r*cos(theta), r*sin(theta), height]');
    cameras{i} = gtsamSimpleCamera_lookat(t, gtsamPoint3, gtsamPoint3([0,0,1]'), K);
end
odometry = cameras{1}.pose.between(cameras{2}.pose);

poseNoise = gtsamSharedNoiseModel_Sigmas([0.001 0.001 0.001 0.1 0.1 0.1]');
odometryNoise = gtsamSharedNoiseModel_Sigmas([0.001 0.001 0.001 0.1 0.1 0.1]');
pointNoise = gtsamSharedNoiseModel_Sigma(3, 0.1);
measurementNoise = gtsamSharedNoiseModel_Sigma(2, 1.0);

%% Initialize iSAM
isam = visualSLAMISAM(2);
newFactors = visualSLAMGraph;
initialEstimates = visualSLAMValues;
if 1 % add hard constraint
    newFactors.addPoseConstraint(symbol('x',1),cameras{1}.pose);
else
    newFactors.addPosePrior(symbol('x',1), cameras{1}.pose, poseNoise);
end
initialEstimates.insertPose(symbol('x',1), cameras{1}.pose);
% Add visual measurement factors from first pose
for j=1:nPoints
    if 0 % add point priors
    	newFactors.addPointPrior(symbol('l',j), points{j}, pointNoise);
    end
    zij = cameras{i}.project(points{j});
    newFactors.addMeasurement(zij, measurementNoise, symbol('x',1), symbol('l',j), K);
    initialEstimates.insertPoint(symbol('l',j), points{j});
end

%% Run iSAM Loop
for i=2:nCameras
    
    %% Add odometry
    newFactors.addOdometry(symbol('x',i-1), symbol('x',i), odometry, odometryNoise);
    
    %% Add visual measurement factors
    for j=1:nPoints
        zij = cameras{i}.project(points{j});
        newFactors.addMeasurement(zij, measurementNoise, symbol('x',i), symbol('l',j), K);
    end
    
    %% Initial estimates for the new pose. Also initialize points while in the first frame.
    %TODO: this might be suboptimal since "result" is not the fully optimized result
    if (i==2), prevPose = cameras{1}.pose;
    else, prevPose = result.pose(symbol('x',i-1)); end
    initialEstimates.insertPose(symbol('x',i), prevPose.compose(odometry));
    
    %% Update ISAM
    isam.update(newFactors, initialEstimates);
    result = isam.estimate();
    if 0 % re-linearize
        isam.reorder_relinearize();
    end
    
    %% Plot results
    P1 = isam.marginalCovariance(symbol('x',1));
    sqrt(diag(P1))
    h=figure(1);clf
    hold on;
    for j=1:size(points,2)
        P = isam.marginalCovariance(symbol('l',j));
        point_j = result.point(symbol('l',j));
        plot3(point_j.x, point_j.y, point_j.z,'marker','o');
        covarianceEllipse3D([point_j.x;point_j.y;point_j.z],P);
    end
    for ii=1:i
        P = isam.marginalCovariance(symbol('x',ii));
        pose_ii = result.pose(symbol('x',ii));
        plotPose3(pose_ii,P,10);
        if 1 % show ground truth
            plotPose3(cameras{ii}.pose,0.001*eye(6),10);
        end
    end
    axis([-40 40 -40 40 -10 20]);axis equal
    view(2)
    colormap('hot')
    %print(h,'-dpng',sprintf('VisualISAM_%03d.png',i));
    
    %% Reset newFactors and initialEstimates to prepare for the next update
    newFactors = visualSLAMGraph;
    initialEstimates = visualSLAMValues;
end