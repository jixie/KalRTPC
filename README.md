# KalRTPC
 Kalman Filter program for RTPC12

##How to get these source code: 

 git clone git@github.com:jixie/KalRTPC.git 
 or
 git clone https://github.com:jixie/KalRTPC.git 

 This program depends on 3 other software packages:
 1) root, you can download it from "root.cern.ch", any version should be fine.
 2) clhep, I used 2.1.0.1 compiled with vc10 in windows xp, and 2.1.3.1 in CentOS_6.5
 3) KalmanFilter libraries originally from KEK, modified by me. 
 You can also get it from github:  
 git clone git@github.com:jixie/KalmanFilter.git 
 or
 git clone https://github.com:jixie/KalmanFilter.git 
 
 You need to have these 2 enviroment variables set: ${KALMANROOT} and ${CLHEP_BASE_DIR}

##How to compile?

 First of all, you need to define $rtpcsoft.  
 You can simply define it this way:  
   setenv rtpcsoft $home/rtpcsoft
 
 1) install root and clhep    
                      
  setenv ROOTSYS ${rtpcsoft}/root-5.28.00-x86_64                              
  setenv ROOTLIB $ROOTSYS/lib                                                
  setenv PATH ${ROOTSYS}/bin:${PATH}                                         
  setenv LD_LIBRARY_PATH ${ROOTSYS}/lib:${LD_LIBRARY_PATH}     

  setenv  CLHEP_BASE_DIR  ${rtpcsoft}/clhep-2.1.3.1-x86_64
  setenv  LD_LIBRARY_PATH ${CLHEP_BASE_DIR}/lib:${LD_LIBRARY_PATH}

 2) install KalmanFilter

  setenv KALMANROOT   ${rtpcsoft}/KalmanFilter  
  setenv LD_LIBRARY_PATH  ${KALMANROOT}/lib:$LD_LIBRARY_PATH

 cd KalmanFilter, change 'setup.csh' to define where you want to 
 install KalmanFilter package, then change to  KalmanFilter/src,
 type 'make clean; make;make install' to install it.

 3) compile kalRTPC
 
  cd kalRTPC;
  make clean;
  make



========================================================================
##To run this in windows:

vc10 project files are also available. I can provide g77 compiler and 
VC10 project files for clhep. You need to download and install vc10 version 
of root_v5.28 from "root.cern.ch".  Place KalmanFilter and KalRTPC into 
the same directory. Then use the provided vc10 solution file. 
