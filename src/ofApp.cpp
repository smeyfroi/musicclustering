#include "ofApp.h"
#include "ofxTimeMeasurements.h"
#include "dkm.hpp"

//--------------------------------------------------------------
void ofApp::setup(){
  ofxTimeMeasurements::instance()->setEnabled(true);
}

//--------------------------------------------------------------
const int CLUSTER_CENTRES = 6;
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
//      clusterResults = dkm::kmeans_lloyd(clusterSourceData, CLUSTER_CENTRES);
    }
    TS_STOP("update-kmeans");
  }
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofClear(255, 255);

  auto& clusterCentres = std::get<0>(clusterResults);

  ofSetColor(0);
  for (auto& centre : clusterCentres) {
    ofDrawCircle(centre[0]*ofGetWindowWidth(), centre[1]*ofGetWindowHeight(), 20);
  }
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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
