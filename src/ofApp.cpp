#include "ofApp.h"
#include "Constants.h"
#include "ofxTimeMeasurements.h"
#include "dkm.hpp"
// https://github.com/genbattle/dkm/blob/master/include/dkm.hpp

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetVerticalSync(false);
  ofEnableAlphaBlending();
  ofDisableArbTex(); // required for texture2D to work in GLSL, makes texture coords normalized
  ofSetFrameRate(Constants::FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(Constants::FRAME_RATE);

  connectionsFbo.allocate(Constants::CANVAS_WIDTH, Constants::CANVAS_HEIGHT, GL_RGBA32F);
  connectionsFbo.begin();
  ofClear(ofFloatColor{0.0, 0.0, 0.0, 0.0});
  connectionsFbo.end();

  gui.setup(parameters);

  ofxTimeMeasurements::instance()->setEnabled(true);
}

//--------------------------------------------------------------
const int CLUSTER_CENTRES = 14;
const int CLUSTER_SAMPLES_MAX = 5000;
void ofApp::update(){
  TS_START("update-audoanalysis");
  audioDataProcessorPtr->update();
  TS_STOP("update-audoanalysis");

  if (audioDataProcessorPtr->isDataValid()) {
    TS_START("update-kmeans");
    float s = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::pitch, 700.0, 1300.0);
    float t = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare, 400.0, 4000.0, false);
    clusterSourceData.push_back({ s, t });
    if (clusterSourceData.size() > CLUSTER_CENTRES) {
      dkm::clustering_parameters<float> params(CLUSTER_CENTRES); // to specify a stable seed
      params.set_random_seed(1000);
      clusterResults = dkm::kmeans_lloyd(clusterSourceData, params);
    }
    TS_STOP("update-kmeans");
    
    TS_START("update-connections");
    auto& clusterCentres = std::get<0>(clusterResults);
    connectionsFbo.begin();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 0.01));
//    ofDrawRectangle(0, 0, connectionsFbo.getWidth(), connectionsFbo.getHeight());
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(ofFloatColor(0.05, 0.05, 0.05, 0.1));
    for (auto& centre1 : clusterCentres) {
      for (auto& centre2 : clusterCentres) {
        float x1 = centre1[0]; float y1 = centre1[1];
        float x2 = centre2[0]; float y2 = centre2[1];
        if (x1 == x2 && y1 == y2) continue;
        ofDrawLine(x1*Constants::CANVAS_WIDTH, y1*Constants::CANVAS_HEIGHT, x2*Constants::CANVAS_WIDTH, y2*Constants::CANVAS_HEIGHT);
      }
    }
    connectionsFbo.end();
    TS_STOP("update-connections");
  }
  
  if (clusterSourceData.size() > CLUSTER_SAMPLES_MAX + 50) {
    clusterSourceData.erase(clusterSourceData.end()-50, clusterSourceData.end());
  }
}

//--------------------------------------------------------------
void ofApp::draw() {
  ofPushStyle();

  ofClear(255, 255);
  
  {
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetFloatColor(1.0, 1.0, 1.0, 1.0);
    connectionsFbo.draw(0, 0, Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT);
  }

//  auto& clusterCentres = std::get<0>(clusterResults);
//  ofSetColor(0);
//  for (auto& centre : clusterCentres) {
//    ofDrawCircle(centre[0]*ofGetWindowWidth(), centre[1]*ofGetWindowHeight(), 20);
//  }
  
  ofPopStyle();

  // gui
  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (audioAnalysisClientPtr->keyPressed(key)) return;
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
