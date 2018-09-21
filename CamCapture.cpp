//
//  CamCapture.cpp
//
//  Created by Stejara Dinulescu on 8/30/18.
//  CamCapture Class
//  A program to handle optical flow and frame-differencing
//  Sends Osc messages containing values from the frame-differencing to a program in Processing in order to produce sound that is driven by motion in the webcam

#include "SquareGenerator.hpp"
#include "Osc.h"


#define MAX_CORNERS 300 //sets up a constant
#define QUALITY_LEVEL 0.005 //whatever corner you find is good
#define MIN_DISTANCE 3 //how far away the corners have to be from each other
#define ELAPSED_FRAMES 300 //number of elapsed frames to check features

#define NUMBER_OF_SQUARES 20


#define LOCAL_PORT 8887
#define DEST_HOST "127.0.0.1"
#define DEST_PORT 8888
#define ELAPSED_FRAMES_ADDR "/OpticalFlowExample/elapsedFrames"
#define ELAPSED_SECS_ADDR "/OpticalFlowExample/elapsedSeconds"
#define SQUARE_ADDR "/OpticalFlowExample/Square"

class CamCapture : public App {
    
public:
    CamCapture();
	void setup() override;
    void mouseMove( MouseEvent event) override;
	void mouseDown( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void keyDown( KeyEvent event) override;
    
    void update() override;
    void draw() override;
    
    void sendOSC( string, float );
    void sendSquareOSC( string, float, float, float );
    
    
protected:
    CaptureRef mCamCapture; //pointer to a CamCapture object
    gl::TextureRef mTexture; //pointer to a texture object
    ci::SurfaceRef mSurface;
    
    cv::Mat mPrevFrame, mCurrFrame, mBGFrame, mFrameDiff;
    vector<cv::Point2f> mPrevFeatures, mFeatures;
    vector<uint8_t> mFeatureStatuses;
    vector<float> errors; //unsigned integers

    void opticalFlow();
    cv::Mat frameDifferencing(cv::Mat frame);
    void frameDifference();
    void displayBSDiff();
    void drawBGDots();
    
    SquareFrameDiff squareDiff;
    osc::SenderUdp mSender;
};

CamCapture::CamCapture() : mSender(LOCAL_PORT, DEST_HOST, DEST_PORT) {}

void CamCapture::setup()
{
    //assigns width and height of rectangle divisions
    squareDiff.divideScreen(NUMBER_OF_SQUARES);
    
    try
    {
        mCamCapture = Capture::create(640, 480); //creates the CamCapture using default camera (webcam)
        mCamCapture->start(); //start the CamCapture (starts the webcam)
    } catch (ci::Exception &e)
    {
        CI_LOG_EXCEPTION("Failed to init capture", e); //if it fails, it will log on the console
    }
    
    //check if binds to local destination port
    try
    {
        mSender.bind();
    } catch(osc::Exception &e)
    {
        CI_LOG_E( "Error binding" << e.what() << " val: " << e.value() );
        quit();
    }
}

void CamCapture::sendOSC( string address, float value )
{
    osc::Message msg;
    msg.setAddress(address); //sets address of the message
    msg.append(value); //adds a parameter to the message

    mSender.send(msg);
}

void CamCapture::sendSquareOSC( string address, float maxSquareMotion, float maxSquareX, float maxSquareY ) //sends Osc messages containing square values
{
    osc::Message msg;
    msg.setAddress(address); //sets address of the message
    msg.append(maxSquareMotion); //adds a parameter to the message
    msg.append(maxSquareX);
    msg.append(maxSquareY);
    
    //cout << "maxMotion: " << maxSquareMotion << " maxX: " << maxSquareY << " maxY: " << maxSquareY << endl;

    mSender.send(msg);
}

void CamCapture::mouseMove( MouseEvent event ) {}

void CamCapture::mouseDown( MouseEvent event ) {}

void CamCapture::mouseUp( MouseEvent event ) {}

void CamCapture::mouseDrag( MouseEvent event ) {}

void CamCapture::keyDown( KeyEvent event )
{
    if (event.getChar() == ' ') //if space bar is down, take a new frame
    {
        mBGFrame = mCurrFrame;
        cv::GaussianBlur(mBGFrame, mBGFrame, cv::Size(5, 5), 0);
    }
    
//    if (event.getChar() == 'd' )  { keyPressed = 'd'; }
}

cv::Mat CamCapture::frameDifferencing(cv::Mat frame) //frame differencing with currFrame
{
    cv::Mat input, outputImg;
    cv::GaussianBlur(mCurrFrame, input, cv::Size(5, 5), 0);
    cv::absdiff(input, frame, outputImg);
    cv::threshold(outputImg, outputImg, 50, 255, cv::THRESH_BINARY);
    return outputImg;
}

void CamCapture::frameDifference() //for differencing with prev frame
{
    if(!mSurface || !mCurrFrame.data) return ;
    if (mPrevFrame.data) { mFrameDiff = frameDifferencing(mPrevFrame); }
    mPrevFrame = mCurrFrame;
}

void CamCapture::displayBSDiff()
{
    if ( !mCurrFrame.data || !mBGFrame.data ) return; //make sure we have set these frames
    //convert matrix to a surface, convert the surface to a texture, then draw the texture
    cv::Mat output = frameDifferencing(mBGFrame); //storing the difference into a matrix called "output"
    ci::Surface sur = fromOcv( output ); //create a surface
    gl::TextureRef tex = gl::Texture::create( sur ); //create a texture
    gl::draw( tex ); //draw the texture
}

void CamCapture::update()
{
    if (mCamCapture && mCamCapture->checkNewFrame()) //is there a new image?
    {
        mSurface = mCamCapture->getSurface(); //will get its most recent surface/whatever it is capturing
        mCurrFrame = toOcv( Channel( *mSurface ) );
        if ( !mTexture ) //if texture doesn't exist
        {
            mTexture = gl::Texture::create( *mSurface ); //create a texture from the surface that we got from the camera
        } else {
            mTexture->update( *mSurface ); //if it does exist, update the surface
        }
    }
    
    frameDifference();
    
    if (mFrameDiff.data) { squareDiff.countPixels(mFrameDiff); } //count the pixels for frame differencing
    
//    sendOSC( ELAPSED_FRAMES_ADDR, getElapsedFrames() );
//    sendOSC( ELAPSED_SECS_ADDR, getElapsedSeconds() );
    sendSquareOSC( SQUARE_ADDR, squareDiff.getMotionValue(), squareDiff.getMaxXValue(), squareDiff.getMaxYValue() );

}

void CamCapture::draw()
{
    gl::clear( Color( 0, 0, 0 ) );
    squareDiff.displaySquares();
    //gl::draw(gl::Texture::create(fromOcv(mFrameDiff)));
    
    
}

CINDER_APP( CamCapture, RendererGl )
