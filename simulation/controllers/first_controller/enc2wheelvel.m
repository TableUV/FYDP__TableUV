function W = enc2wheelvel(left_enc,right_enc,WHEEL_RADIUS,TIME_STEP)
    %enc2wheelvel: convert encoder values (radians in webots) to wheel vel
    left_radPsec = left_enc / (TIME_STEP * 0.001);
    right_radPsec = right_enc / (TIME_STEP * 0.001);
     
    W = [left_radPsec * WHEEL_RADIUS;
         right_radPsec * WHEEL_RADIUS];
end

