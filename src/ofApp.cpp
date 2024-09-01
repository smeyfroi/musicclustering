#include "ofApp.h"
#include "Constants.h"
#include "ofxTimeMeasurements.h"
#include "dkm.hpp"
// https://github.com/genbattle/dkm/blob/master/include/dkm.hpp

//--------------------------------------------------------------
const int SOM_WIDTH = 128;
const int SOM_HEIGHT = 128;
void ofApp::setup(){
  ofSetVerticalSync(false);
  ofEnableAlphaBlending();
  ofDisableArbTex(); // required for texture2D to work in GLSL, makes texture coords normalized
  ofSetFrameRate(Constants::FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(Constants::FRAME_RATE);

//  connectionsFbo.allocate(Constants::CANVAS_WIDTH, Constants::CANVAS_HEIGHT, GL_RGBA32F);
//  connectionsFbo.begin();
//  ofClear(ofFloatColor{0.0, 0.0, 0.0, 0.0});
//  connectionsFbo.end();

  double minInstance[3] = { 0.0, 0.0, 0.0 };
  double maxInstance[3] = { 1.0, 1.0, 1.0 };
  som.setFeaturesRange(3, minInstance, maxInstance);
  som.setMapSize(SOM_WIDTH, SOM_HEIGHT); // can go to 3 dimensions
  som.setInitialLearningRate(0.1);
  som.setNumIterations(3000);
  som.setup();
  
  fluidSimulation.setup({ Constants::FLUID_WIDTH, Constants::FLUID_HEIGHT });

  parameters.add(fluidSimulation.getParameterGroup());
  gui.setup(parameters);

  ofxTimeMeasurements::instance()->setEnabled(true);
}

//--------------------------------------------------------------
const int CLUSTER_CENTRES = 8; //14;
const int CLUSTER_SAMPLES_MAX = 3000;
void ofApp::update(){
  TS_START("update-audoanalysis");
  audioDataProcessorPtr->update();
  TS_STOP("update-audoanalysis");

  if (audioDataProcessorPtr->isDataValid()) {
    float s = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::pitch, 200.0, 1800.0);// 700.0, 1300.0);
    float t = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare, 000.0, 4600.0); ////400.0, 4000.0, false);
    float u = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::spectralKurtosis, 0.0, 25.0);
    float v = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::spectralCentroid, 0.4, 6.0);

    TS_START("update-kmeans");
    clusterSourceData.push_back({ s, t });
    if (clusterSourceData.size() > CLUSTER_CENTRES) {
      dkm::clustering_parameters<float> params(CLUSTER_CENTRES); // to specify a stable seed
      params.set_random_seed(1000);
      clusterResults = dkm::kmeans_lloyd(clusterSourceData, params);
    }
    TS_STOP("update-kmeans");

    TS_START("update-som");
    double instance[3] = {
//      static_cast<double>(s),
      static_cast<double>(s),
      static_cast<double>(t),
      static_cast<double>(v)
    };
    som.updateMap(instance);
    TS_STOP("update-som");

    TS_START("update-connections");
    auto& clusterCentres = std::get<0>(clusterResults);
//    connectionsFbo.begin();
    fluidSimulation.getFlowValuesFbo().getSource().begin();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 0.01));
//    ofDrawRectangle(0, 0, connectionsFbo.getWidth(), connectionsFbo.getHeight());
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (auto& centre1 : clusterCentres) {
      for (auto& centre2 : clusterCentres) {
        float x1 = centre1[0]; float y1 = centre1[1];
        float x2 = centre2[0]; float y2 = centre2[1];
        if (x1 == x2 && y1 == y2) continue;
        double* somValue = som.getMapAt(x1*SOM_WIDTH, y1*SOM_HEIGHT);
        const float COL_FACTOR = 0.3;
        ofFloatColor color(somValue[0]*COL_FACTOR, somValue[1]*COL_FACTOR, somValue[2]*COL_FACTOR, 0.1);
        ofSetColor(color);
        ofDrawLine(x1*Constants::FLUID_WIDTH, y1*Constants::FLUID_HEIGHT, x2*Constants::FLUID_WIDTH, y2*Constants::FLUID_HEIGHT);
      }
    }
    fluidSimulation.getFlowValuesFbo().getSource().end();
//    connectionsFbo.end();
    TS_STOP("update-connections");
  }
  
  if (clusterSourceData.size() > CLUSTER_SAMPLES_MAX + 50) {
    clusterSourceData.erase(clusterSourceData.end()-50, clusterSourceData.end());
  }
  
  {
    TS_START("update-fluid");
    auto& clusterCentres = std::get<0>(clusterResults);
    for (auto& centre : clusterCentres) {
      float x = centre[0]; float y = centre[1];
      double* somValue = som.getMapAt(x * SOM_WIDTH, y * SOM_HEIGHT);
      const float COL_FACTOR = 0.008;
      ofFloatColor color = ofFloatColor(somValue[0], somValue[1], somValue[2], 0.005) * COL_FACTOR;
      FluidSimulation::Impulse impulse {
        { x * Constants::FLUID_WIDTH, y * Constants::FLUID_HEIGHT },
        Constants::FLUID_WIDTH * 0.05, // radius
        { 0.0, 0.0 }, // velocity
        0.0005, // radialVelocity
        color,
        10.0 // temperature
      };
      fluidSimulation.applyImpulse(impulse);
    }
    fluidSimulation.update();
    TS_STOP("update-fluid");
  }
}

//--------------------------------------------------------------
void ofApp::draw() {
  ofPushStyle();

  ofClear(255, 255);
  
  {
    fluidSimulation.draw(0, 0, Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT);
  }
  
//  {
//    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
//    ofSetFloatColor(1.0, 1.0, 1.0, 1.0);
//    connectionsFbo.draw(0, 0, Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT);
//  }

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
