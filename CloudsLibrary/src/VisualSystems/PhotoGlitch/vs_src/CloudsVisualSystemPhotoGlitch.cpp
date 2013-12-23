//
//  CloudsVisualSystemPhotoGlitch.cpp
//

#include "CloudsVisualSystemPhotoGlitch.h"


//#include "CloudsRGBDVideoPlayer.h"
//#ifdef AVF_PLAYER
//#include "ofxAVFVideoPlayer.h"
//#endif

static int kVertsPerCell   = 4;
static int kCoordsPerVert  = 2;
static int kIndicesPerCell = 6;

static ofxEasingQuad easing;

PGCell * CloudsVisualSystemPhotoGlitch::cells;
PGCell * CloudsVisualSystemPhotoGlitch::targetCells;

bool CloudsVisualSystemPhotoGlitch::sortIdxForHue(int i, int j) {
    return (cells[i].avgColor.getHue() < cells[j].avgColor.getHue());
}

bool CloudsVisualSystemPhotoGlitch::sortIdxForHueTarget(int i, int j) {
    return (targetCells[i].avgColor.getHue() < targetCells[j].avgColor.getHue());
}

bool CloudsVisualSystemPhotoGlitch::sortIdxForBrightnessTarget(int i, int j) {
    return (targetCells[i].avgColor.getBrightness() < targetCells[j].avgColor.getBrightness());
}

bool CloudsVisualSystemPhotoGlitch::sortIdxForBri(int i, int j) {
    return (cells[i].avgColor.getBrightness() < cells[j].avgColor.getBrightness());
}




bool sortIdxForHueTargetNew(compareObj obj1,compareObj obj2){
    return (obj1.cell->avgColor.getHue() < obj2.cell->avgColor.getHue());
}

bool sortIdxForBrightnessTargetNew(compareObj obj1,compareObj obj2){
    
    return (obj1.cell->avgColor.getBrightness() < obj2.cell->avgColor.getBrightness());
}




//These methods let us add custom GUI parameters and respond to their events
void CloudsVisualSystemPhotoGlitch::selfSetupGui()
{
	customGui = new ofxUISuperCanvas("PhotoGlitch", gui);
	customGui->copyCanvasStyle(gui);
	customGui->copyCanvasProperties(gui);
	customGui->setName("PhotoGlitch");
	customGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    customGui->addSpacer();
    vector<string> imageNames;
    for (int i = 0; i < imagesDir.size(); i++) {
        imageNames.push_back(imagesDir.getName(i));
    }
    
    vector<string> targetImageNames;
    for(int i =0; i<targetImagesDir.size(); i++){
        targetImageNames.push_back(targetImagesDir.getName(i));
        gp1.targetImageNames.push_back(targetImagesDir.getName(i));
        gp2.targetImageNames.push_back(targetImagesDir.getName(i));
    }
    
    customGui->addSpacer();
    customGui->addIntSlider("NUM COLS", 1, 400, &numDivCols);
    customGui->addIntSlider("NUM ROWS", 1, 400, &numDivRows);
    
    customGui->addSpacer();
    customGui->addToggle("USE COLORS", &bUseColors);
    customGui->addToggle("USE TEXTURE", &bUseTexture);
    
    customGui->addSpacer();
    customGui->addLabel("DEBUG STUFF");
    customGui->addToggle("DRAW TARGET", &bDrawTarget);
    customGui->addToggle("DRAW BACKGROUND", &bDrawBackground);
    
    customGui->addLabel("SOURCE SORT MODES");
	customGui->addToggle("SHUFFLE", &bShouldShuffle);
    customGui->addToggle("SORT HUE", &bShouldSortHue);
    customGui->addToggle("SORT BRI", &bShouldSortBri);
    customGui->addToggle("RANDOM SORT", &bRandomSort);
    customGui->addToggle("REORDER", &bShouldReorder);
    
    customGui->addLabel("ANIMATION PARAMS");
    customGui->addIntSlider("TWEEN DURATION", 1, 1000, &tweenDuration);
    customGui->addIntSlider("TWEEN DELAY", 0, 1000, &tweenDelay);
    customGui->addToggle("PERPENDICULAR", &bDoPerpendicular);
    customGui->addDropDownList("SOURCE IMAGES", imageNames);
    
    addTargetToUI(customGui,"1", gp1);
    addTargetToUI(customGui,"2", gp2);
    
    customGui->addButton("ANIMATE", false);
    
	ofAddListener(customGui->newGUIEvent, this, &CloudsVisualSystemPhotoGlitch::selfGuiEvent);
	guis.push_back(customGui);
	guimap[customGui->getName()] = customGui;
}

void CloudsVisualSystemPhotoGlitch::addTargetToUI(ofxUISuperCanvas* gui,string suffix, glitchParams& params ){
    
    gui->addLabel("TARGET PARAMS");
    gui->addToggle("ENABLE", &params.enable);
    gui->addToggle("SORT BY HUE", &params.sortByHue);
    gui->addToggle("SORT BY SAT", &params.sortByBrightness);
    gui->addDropDownList("TARGET IMAGES " + suffix, params.targetImageNames);
    gui->addButton("UPDATE TARGET " + suffix,false);
    
}

int CloudsVisualSystemPhotoGlitch::getTargetFileName(ofxUISuperCanvas * gui, int targetId){
    ofxUIDropDownList* menu = (ofxUIDropDownList*) gui->getWidget("TARGET IMAGES "+ ofToString(targetId));
    vector<ofxUILabelToggle*> files = menu->getToggles();
    
    //    cout<<fileNames->getValue()<<endl;
    for (int i=0 ; i<files.size(); i++) {
        for (int j =0; j < targetImagesDir.numFiles(); j++){
            ofxUILabel* l = files[i]->getLabel();
            
            if (l->getLabel() == targetImagesDir.getName(j) && files[i]->getValue()) {
                cout<<"current file for target : "<<targetId<<" is "<< targetImagesDir.getName(j)<<endl;
                return j;
            }
        }
    }
    
    return -1;

}
void CloudsVisualSystemPhotoGlitch::selfGuiEvent(ofxUIEventArgs &e)
{
    
    if (e.getName() == "NUM COLS" || e.getName() == "NUM ROWS") {
        bShouldGenerate = true;
    }
    else if(e.getName() == "UPDATE TARGET 1"){
        cout<<"Updating Target 1"<<endl;
    }
    else if(e.getName() == "UPDATE TARGET 2"){
        cout<<"Updating Target 2"<<endl;
    }
    else if(e.getName() == "ANIMATE"){
        ofxUIButton* b = (ofxUIButton*) e.widget;

        if(b->getValue()){
            if (gp1.enable && !gp2.enable) {
                int imgIdx = getTargetFileName(customGui, 1);
                
                if( imgIdx != -1){
                    generate(target1, imgIdx);
                    target1.ID = 1;
                    currentTarget = &target1;
                }
                else{
                    cout<<"No image selected or image not found for image 1"<<endl;
                }
                
            }
            else if (gp1.enable && gp2.enable){
                
                int imgIdx = getTargetFileName(customGui, 1);
                int imgIdx2 = getTargetFileName(customGui, 2);
                
                if( imgIdx != -1){
                    generate(target1, imgIdx);
                    target1.ID = 1;
                }
                else{
                    cout<<"No image selected or image not found for image 1"<<endl;
                }
                
                if( imgIdx2 != -1){
                    generate(target2, imgIdx2);
                    target2.ID = 2;
                }
                else{
                    cout<<"No image selected or image not found for image 2"<<endl;
                }
                currentTarget = &target1;
            }
            else if( (! gp1.enable) && gp2.enable ){
                int imgIdx2 = getTargetFileName(customGui, 2);
                
                if( imgIdx2 != -1){
                    generate(target2, imgIdx2);
                    target2.ID = 2;
                }
                else{
                    cout<<"No image selected or image not found for image id 2"<<endl;
                }
                
                currentTarget = &target2;
                
            }
        }

    }
    else {
        string name = e.widget->getName();
        if(e.widget->getParent()->getName() == "SOURCE IMAGES" ){
            // Look through the files dropdown for a match.
            for (int i = 0; i < imagesDir.numFiles(); i++) {
                if (name == imagesDir.getName(i) && ((ofxUIToggle *)e.widget)->getValue()) {
                    cout<<"source img name : "<<name<<endl;
                    selectedSrcImageIdx = i;
                    bShouldGenerate = true;
                    break;
                }
            }
        }

    }
    
}

//Use system gui for global or logical settings, for exmpl
void CloudsVisualSystemPhotoGlitch::selfSetupSystemGui(){
	
}

void CloudsVisualSystemPhotoGlitch::guiSystemEvent(ofxUIEventArgs &e){
	
}

//use render gui for display settings, like changing colors
void CloudsVisualSystemPhotoGlitch::selfSetupRenderGui(){
    
}

void CloudsVisualSystemPhotoGlitch::guiRenderEvent(ofxUIEventArgs &e){
	
}

// selfSetup is called when the visual system is first instantiated
// This will be called during a "loading" screen, so any big images or
// geometry should be loaded here
void CloudsVisualSystemPhotoGlitch::selfSetup()
{
    bIs2D = true;
    
    // Set defaults.
    bUseColors  = false;
    bUseTexture = true;
    
    bShouldShuffle = false;
    bShouldSortHue = false;
    bShouldSortBri = false;
    bShouldReorder = false;
    bShouldSortTarget = false;
    bShouldSortTargetBri = false;
    bCurrentlyAnimating = false;
    
    bDoPerpendicular = false;
    
    tweenDuration = 200;
    tweenDelay = 0;
    
    numDivCols = 20;
    numDivRows = 20;
    
    imagesDir.listDir(getVisualSystemDataPath() + "sourceImages" );
    imagesDir.sort();
    
    targetImagesDir.listDir(getVisualSystemDataPath() + "targetImages" );
    targetImagesDir.sort();
    selectedSrcImageIdx = 0;
    selectedTargetImageIdx = 0;
    currentTargetIndex = 0;
    
    screenRect = ofRectangle(0, 0, ofGetWidth(), ofGetHeight());
    bShouldGenerate = true;
    //    ofEnableAlphaBlending();
}

void CloudsVisualSystemPhotoGlitch::clearSource()
{
    //TODO: Put this in the right place.
    currentTargetIndex = 0;
    if (cells != NULL) {
        delete [] cells;
        cells = NULL;
    }
    
    if (verts != NULL) {
        delete [] verts;
        verts = NULL;
    }
    
    if (texCoords != NULL) {
        delete [] texCoords;
        texCoords = NULL;
    }
    
    if (colors != NULL) {
        delete [] colors;
        colors = NULL;
    }
    
    if (indices != NULL) {
        delete [] indices;
        indices = NULL;
    }
    
    tex.clear();
}

void CloudsVisualSystemPhotoGlitch::generate(PhotoGlitch& pg,int imgIndex){
    
    
    pg.tex.loadImage(targetImagesDir.getPath(imgIndex));
    
    ofPixels pixels = pg.tex.getPixelsRef();
    
    numCells = numDivCols * numDivRows;
    
    pg.cells = new PGCell[numCells];
    
    screenSliceWidth = ofGetWidth() / (float)numDivCols;
    screenSliceHeight = ofGetHeight() / (float)numDivRows;
    texSliceWidth = pg.tex.getWidth() / (float)numDivCols;
    texSliceHeight = pg.tex.getHeight() / (float)numDivRows;
    
    // Build the mesh data.
    pg.numVerts = numCells * kVertsPerCell;
    pg.verts = new GLfloat[pg.numVerts * kCoordsPerVert];
    pg.texCoords = new GLfloat[pg.numVerts * 2];
    
    pg.colors = new GLfloat[pg.numVerts * 4];
    
    pg.numIndices = numCells * kIndicesPerCell;
    pg.indices = new GLuint[pg.numIndices];
    
    for (int j = 0; j < numDivRows; j++) {
        for (int i = 0; i < numDivCols; i++) {
            int idx = j * numDivCols + i;
            
            // Add verts. Same for both source and target
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 0] = (i + 0) * screenSliceWidth;
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 1] = (j + 0) * screenSliceHeight;
            
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 2] = (i + 1) * screenSliceWidth;
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 3] = (j + 0) * screenSliceHeight;
            
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 4] = (i + 0) * screenSliceWidth;
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 5] = (j + 1) * screenSliceHeight;
            
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 6] = (i + 1) * screenSliceWidth;
            pg.verts[idx * kVertsPerCell * kCoordsPerVert + 7] = (j + 1) * screenSliceHeight;
            
            // Add tex coords.
            pg.texCoords[idx * kVertsPerCell * 2 + 0] = (i + 0) * texSliceWidth;
            pg.texCoords[idx * kVertsPerCell * 2 + 1] = (j + 0) * texSliceHeight;
            
            pg.texCoords[idx * kVertsPerCell * 2 + 2] = (i + 1) * texSliceWidth;
            pg.texCoords[idx * kVertsPerCell * 2 + 3] = (j + 0) * texSliceHeight;
            
            pg.texCoords[idx * kVertsPerCell * 2 + 4] = (i + 0) * texSliceWidth;
            pg.texCoords[idx * kVertsPerCell * 2 + 5] = (j + 1) * texSliceHeight;
            
            pg.texCoords[idx * kVertsPerCell * 2 + 6] = (i + 1) * texSliceWidth;
            pg.texCoords[idx * kVertsPerCell * 2 + 7] = (j + 1) * texSliceHeight;
            
            // Add indices for triangles.
            pg.indices[idx * kIndicesPerCell + 0] = idx * kVertsPerCell + 0;  // top-left
            pg.indices[idx * kIndicesPerCell + 1] = idx * kVertsPerCell + 1;  // top-right
            pg.indices[idx * kIndicesPerCell + 2] = idx * kVertsPerCell + 2;  // bottom-left
            
            pg.indices[idx * kIndicesPerCell + 3] = idx * kVertsPerCell + 1;  // top-right
            pg.indices[idx * kIndicesPerCell + 4] = idx * kVertsPerCell + 3;  // bottom-right
            pg.indices[idx * kIndicesPerCell + 5] = idx * kVertsPerCell + 2;  // bottom-left
            
            // Calculate the average source color.
            int avgR = 0, avgG = 0, avgB = 0, avgA = 0;
            for (int y = (j + 0) * texSliceHeight; y < (j + 1) * texSliceHeight; y++) {
                for (int x = (i + 0) * texSliceWidth; x < (i + 1) * texSliceWidth; x++) {
                    ofColor c = pixels.getColor(x, y);
                    avgR += c.r;
                    avgG += c.g;
                    avgB += c.b;
                    avgA += c.a;
                }
            }
            
            pg.cells[idx].avgColor.set((avgR / (texSliceWidth * texSliceHeight)) / 255.0f,
                                       (avgG / (texSliceWidth * texSliceHeight)) / 255.0f,
                                       (avgB / (texSliceWidth * texSliceHeight)) / 255.0f,
                                       (avgA / (texSliceWidth * texSliceHeight)) / 255.0f);
            
            if (pg.cells[idx].avgColor.getBrightness() < 0.1) {
                pg.cells[idx].avgColor.setHsb(ofRandomuf(), ofRandomuf(),ofRandomuf(), 0.0);
            }
            
            // Add colors.
            pg.colors[idx * kVertsPerCell * 4 +  0] = pg.cells[idx].avgColor.r;
            pg.colors[idx * kVertsPerCell * 4 +  1] = pg.cells[idx].avgColor.g;
            pg.colors[idx * kVertsPerCell * 4 +  2] = pg.cells[idx].avgColor.b;
            pg.colors[idx * kVertsPerCell * 4 +  3] = pg.cells[idx].avgColor.a;
            
            pg.colors[idx * kVertsPerCell * 4 +  4] = pg.cells[idx].avgColor.r;
            pg.colors[idx * kVertsPerCell * 4 +  5] = pg.cells[idx].avgColor.g;
            pg.colors[idx * kVertsPerCell * 4 +  6] = pg.cells[idx].avgColor.b;
            pg.colors[idx * kVertsPerCell * 4 +  7] = pg.cells[idx].avgColor.a;
            
            pg.colors[idx * kVertsPerCell * 4 +  8] = pg.cells[idx].avgColor.r;
            pg.colors[idx * kVertsPerCell * 4 +  9] = pg.cells[idx].avgColor.g;
            pg.colors[idx * kVertsPerCell * 4 + 10] = pg.cells[idx].avgColor.b;
            pg.colors[idx * kVertsPerCell * 4 + 11] = pg.cells[idx].avgColor.a;
            
            pg.colors[idx * kVertsPerCell * 4 + 12] = pg.cells[idx].avgColor.r;
            pg.colors[idx * kVertsPerCell * 4 + 13] = pg.cells[idx].avgColor.g;
            pg.colors[idx * kVertsPerCell * 4 + 14] = pg.cells[idx].avgColor.b;
            pg.colors[idx * kVertsPerCell * 4 + 15] = pg.cells[idx].avgColor.a;
            
            // Save the cell struct.
            pg.cells[idx].idx = idx;
            pg.cells[idx].col = pg.cells[idx].origCol = i;
            pg.cells[idx].row = pg.cells[idx].origRow = j;
        }
    }
    pg.vbo.setVertexData(pg.verts, kCoordsPerVert, pg.numVerts, GL_STREAM_DRAW, kCoordsPerVert * sizeof(GLfloat));
    pg.vbo.setTexCoordData(pg.texCoords, pg.numVerts, GL_STATIC_DRAW, 2 * sizeof(GLfloat));
    pg.vbo.setColorData(pg.colors, pg.numVerts, GL_STATIC_DRAW, 4 * sizeof(GLfloat));
    pg.vbo.setIndexData(pg.indices, pg.numIndices, GL_STATIC_DRAW);
    
    //    photos.push_back(pg);
    
}


void CloudsVisualSystemPhotoGlitch::generateSource()
{
    clearSource();
    
    tex.loadImage(imagesDir.getPath(selectedSrcImageIdx));
    ofPixels pixels = tex.getPixelsRef();
    
    numCells = numDivCols * numDivRows;
    
    cells = new PGCell[numCells];
    //    targetCells = new PGCell[numCells];
    
    screenSliceWidth = ofGetWidth() / (float)numDivCols;
    screenSliceHeight = ofGetHeight() / (float)numDivRows;
    texSliceWidth = tex.getWidth() / (float)numDivCols;
    texSliceHeight = tex.getHeight() / (float)numDivRows;
    
    // Build the mesh data.
    numVerts = numCells * kVertsPerCell;
    verts = new GLfloat[numVerts * kCoordsPerVert];
    texCoords = new GLfloat[numVerts * 2];
    
    colors = new GLfloat[numVerts * 4];
    
    numIndices = numCells * kIndicesPerCell;
    indices = new GLuint[numIndices];
    
    for (int j = 0; j < numDivRows; j++) {
        for (int i = 0; i < numDivCols; i++) {
            int idx = j * numDivCols + i;
            
            // Add verts. Same for both source and target
            verts[idx * kVertsPerCell * kCoordsPerVert + 0] = (i + 0) * screenSliceWidth;
            verts[idx * kVertsPerCell * kCoordsPerVert + 1] = (j + 0) * screenSliceHeight;
            
            verts[idx * kVertsPerCell * kCoordsPerVert + 2] = (i + 1) * screenSliceWidth;
            verts[idx * kVertsPerCell * kCoordsPerVert + 3] = (j + 0) * screenSliceHeight;
            
            verts[idx * kVertsPerCell * kCoordsPerVert + 4] = (i + 0) * screenSliceWidth;
            verts[idx * kVertsPerCell * kCoordsPerVert + 5] = (j + 1) * screenSliceHeight;
            
            verts[idx * kVertsPerCell * kCoordsPerVert + 6] = (i + 1) * screenSliceWidth;
            verts[idx * kVertsPerCell * kCoordsPerVert + 7] = (j + 1) * screenSliceHeight;
            
            
            // Add tex coords.
            texCoords[idx * kVertsPerCell * 2 + 0] = (i + 0) * texSliceWidth;
            texCoords[idx * kVertsPerCell * 2 + 1] = (j + 0) * texSliceHeight;
            
            texCoords[idx * kVertsPerCell * 2 + 2] = (i + 1) * texSliceWidth;
            texCoords[idx * kVertsPerCell * 2 + 3] = (j + 0) * texSliceHeight;
            
            texCoords[idx * kVertsPerCell * 2 + 4] = (i + 0) * texSliceWidth;
            texCoords[idx * kVertsPerCell * 2 + 5] = (j + 1) * texSliceHeight;
            
            texCoords[idx * kVertsPerCell * 2 + 6] = (i + 1) * texSliceWidth;
            texCoords[idx * kVertsPerCell * 2 + 7] = (j + 1) * texSliceHeight;
            
            
            // Add indices for triangles.
            indices[idx * kIndicesPerCell + 0] = idx * kVertsPerCell + 0;  // top-left
            indices[idx * kIndicesPerCell + 1] = idx * kVertsPerCell + 1;  // top-right
            indices[idx * kIndicesPerCell + 2] = idx * kVertsPerCell + 2;  // bottom-left
            
            indices[idx * kIndicesPerCell + 3] = idx * kVertsPerCell + 1;  // top-right
            indices[idx * kIndicesPerCell + 4] = idx * kVertsPerCell + 3;  // bottom-right
            indices[idx * kIndicesPerCell + 5] = idx * kVertsPerCell + 2;  // bottom-left
            
            // Calculate the average source color.
            int avgR = 0, avgG = 0, avgB = 0, avgA = 0;
            for (int y = (j + 0) * texSliceHeight; y < (j + 1) * texSliceHeight; y++) {
                for (int x = (i + 0) * texSliceWidth; x < (i + 1) * texSliceWidth; x++) {
                    ofColor c = pixels.getColor(x, y);
                    avgR += c.r;
                    avgG += c.g;
                    avgB += c.b;
                    avgA += c.a;
                }
            }
            cells[idx].avgColor.set((avgR / (texSliceWidth * texSliceHeight)) / 255.0f,
                                    (avgG / (texSliceWidth * texSliceHeight)) / 255.0f,
                                    (avgB / (texSliceWidth * texSliceHeight)) / 255.0f,
                                    (avgA / (texSliceWidth * texSliceHeight)) / 255.0f);
            
            //            cout<<cells[idx].avgColor.getBrightness()<<endl;
            if (cells[idx].avgColor.getBrightness() < 0.1) {
                cells[idx].avgColor.setHsb(ofRandomuf(), ofRandomuf(),ofRandomuf(), 0.0);
                cout<<cells[idx].avgColor.getHue()<<endl;
            }
            
            // Add colors.
            colors[idx * kVertsPerCell * 4 +  0] = cells[idx].avgColor.r;
            colors[idx * kVertsPerCell * 4 +  1] = cells[idx].avgColor.g;
            colors[idx * kVertsPerCell * 4 +  2] = cells[idx].avgColor.b;
            colors[idx * kVertsPerCell * 4 +  3] = cells[idx].avgColor.a;
            
            colors[idx * kVertsPerCell * 4 +  4] = cells[idx].avgColor.r;
            colors[idx * kVertsPerCell * 4 +  5] = cells[idx].avgColor.g;
            colors[idx * kVertsPerCell * 4 +  6] = cells[idx].avgColor.b;
            colors[idx * kVertsPerCell * 4 +  7] = cells[idx].avgColor.a;
            
            colors[idx * kVertsPerCell * 4 +  8] = cells[idx].avgColor.r;
            colors[idx * kVertsPerCell * 4 +  9] = cells[idx].avgColor.g;
            colors[idx * kVertsPerCell * 4 + 10] = cells[idx].avgColor.b;
            colors[idx * kVertsPerCell * 4 + 11] = cells[idx].avgColor.a;
            
            colors[idx * kVertsPerCell * 4 + 12] = cells[idx].avgColor.r;
            colors[idx * kVertsPerCell * 4 + 13] = cells[idx].avgColor.g;
            colors[idx * kVertsPerCell * 4 + 14] = cells[idx].avgColor.b;
            colors[idx * kVertsPerCell * 4 + 15] = cells[idx].avgColor.a;
            
            // Save the cell struct.
            cells[idx].idx = idx;
            cells[idx].col = cells[idx].origCol = i;
            cells[idx].row = cells[idx].origRow = j;
            
        }
    }
    
    vbo.setVertexData(verts, kCoordsPerVert, numVerts, GL_STREAM_DRAW, kCoordsPerVert * sizeof(GLfloat));
    vbo.setTexCoordData(texCoords, numVerts, GL_STATIC_DRAW, 2 * sizeof(GLfloat));
    vbo.setColorData(colors, numVerts, GL_STATIC_DRAW, 4 * sizeof(GLfloat));
    vbo.setIndexData(indices, numIndices, GL_STATIC_DRAW);
    
    //    bShouldReorder = true;
}


void CloudsVisualSystemPhotoGlitch::clearTarget(){
    
    if (targetCells != NULL) {
        delete [] targetCells;
        targetCells = NULL;
    }
    
    if (targetVerts != NULL) {
        delete [] targetVerts;
        targetVerts = NULL;
    }
    
    if( targetTexCoords != NULL){
        delete [] targetTexCoords;
        targetTexCoords = NULL;
    }
    
    if (targetColors != NULL) {
        delete [] targetColors;
        targetColors = NULL;
    }
    
    if (targetIndices != NULL) {
        delete [] targetIndices;
        targetIndices = NULL;
    }
    targetTex.clear();
}
// selfPresetLoaded is called whenever a new preset is triggered
// it'll be called right before selfBegin() and you may wish to
// refresh anything that a preset may offset, such as stored colors or particles
void CloudsVisualSystemPhotoGlitch::selfPresetLoaded(string presetPath)
{
    
}

// selfBegin is called when the system is ready to be shown
// this is a good time to prepare for transitions
// but try to keep it light weight as to not cause stuttering
void CloudsVisualSystemPhotoGlitch::selfBegin(){
	
}

//do things like ofRotate/ofTranslate here
//any type of transformation that doesn't have to do with the camera
void CloudsVisualSystemPhotoGlitch::selfSceneTransformation(){
	
}

//normal update call
void CloudsVisualSystemPhotoGlitch::selfUpdate()
{
    if (bShouldGenerate) {
        generateSource();
        //        generate();
        bShouldGenerate = false;
    }
    
    if(bShouldGenerateTargetOnly){
        
        //        generate();
        bShouldGenerateTargetOnly = false;
    }
    
    if (bShouldShuffle) {
        shuffle();
        bShouldShuffle = false;
        bCurrentlyAnimating = true;
        currentTargetIndex = 0;
    }
    if (bShouldSortHue) {
        sortHue();
        bShouldSortHue = false;
        bCurrentlyAnimating = true;
        currentTargetIndex = 0;
    }
    if (bShouldSortBri) {
        sortBri();
        bShouldSortBri = false;
        bCurrentlyAnimating = true;
        currentTargetIndex = 0;
    }
    if (bShouldReorder) {
        reorder();
        bShouldReorder = false;
        bCurrentlyAnimating = true;
        currentTargetIndex = 0;
    }
    if (bShouldSortTarget){
        sortTarget();
        bShouldSortTarget = false;
        bCurrentlyAnimating = true;
        currentTargetIndex = 0;
    }
    if (bShouldSortTargetBri) {
        sortTargetBrightness();
        bShouldSortTargetBri = false;
        bCurrentlyAnimating = true;
        currentTargetIndex = 0;
    }
    
    
    // tween them cells!
    for (int i = 0; i < numCells; i++) {
        int vertIdx = cells[i].idx * kVertsPerCell * kCoordsPerVert;
        
        float tweenX = cells[i].tweenX.update();
        float tweenY = cells[i].tweenY.update();
        
        
        // update the vert data
        // top-left
        verts[vertIdx + 0] = tweenX;
        verts[vertIdx + 1] = tweenY;
        // top-right
        verts[vertIdx + 2] = tweenX + screenSliceWidth;
        verts[vertIdx + 3] = tweenY;
        // bottom-left
        verts[vertIdx + 4] = tweenX;
        verts[vertIdx + 5] = tweenY + screenSliceHeight;
        // bottom-right
        verts[vertIdx + 6] = tweenX + screenSliceWidth;
        verts[vertIdx + 7] = tweenY + screenSliceHeight;
    }
    
    // update that data
    vbo.setVertexData(verts, kCoordsPerVert, numVerts, GL_STREAM_DRAW, kCoordsPerVert * sizeof(GLfloat));
    
    //TODO: Optimise this if it feels too slow
    bool isComplete = false;
    for (int i = 0; i < numCells; i++) {
        int vertIdx = cells[i].idx * kVertsPerCell * kCoordsPerVert;
        
        if(cells[i].tweenX.isRunning() && cells[i].tweenY.isRunning()){
            //            bCurrentlyAnimating = false;
            isComplete  = false;
            break;
        }
        else{
            isComplete  = true;
        }
    }
    
    //    if(isComplete && bCurrentlyAnimating){
    //        bCurrentlyAnimating = false;
    //        if(currentTargetIndex == photos.size()-1){
    //            cout<<"Sequence complete"<<endl;
    //
    //        }
    //        else{
    //            currentTargetIndex++;
    //            cout<<"Going to index : "<<currentTargetIndex<<" of "<<photos.size();
    //            sortTargetBrightness();
    //            bCurrentlyAnimating = true;
    //        }
    //
    //    }
}

// selfDraw draws in 3D using the default ofEasyCamera
// you can change the camera by returning getCameraRef()
void CloudsVisualSystemPhotoGlitch::selfDraw(){
    
}

// draw any debug stuff here
void CloudsVisualSystemPhotoGlitch::selfDrawDebug(){
    
}

// or you can use selfDrawBackground to do 2D drawings that don't use the 3D camera
void CloudsVisualSystemPhotoGlitch::selfDrawBackground()
{
    if (bUseColors) {
        vbo.enableColors();
        //        targetVbo.enableColors();
        //        photos[currentTargetIndex].vbo.enableColors();
    }
    else{
        vbo.disableColors();
        //     targetVbo.disableColors();
        //     photos[currentTargetIndex].vbo.disableColors();
    }
    
    if (bUseTexture){
        vbo.enableTexCoords();
        targetVbo.enableTexCoords();
        //        photos[currentTargetIndex].vbo.enableTexCoords();
    }
    else {
        vbo.disableTexCoords();
        //        targetVbo.disableTexCoords();
        //        photos[currentTargetIndex].vbo.disableTexCoords();
    }
    
    if(bDrawBackground){
        ofPushStyle();
        for (int i = 0; i < numCells; i++) {
            ofSetColor(cells[i].avgColor);
            ofRect(cells[i].col * screenSliceWidth, cells[i].row * screenSliceHeight, screenSliceWidth, screenSliceHeight);
        }
        ofPopStyle();
    }
    
    
    tex.bind();
    vbo.drawElements(GL_TRIANGLES, numIndices);
    tex.unbind();
    
    
    if(bDrawTarget){
        //        photos[currentTargetIndex].tex.bind();
        //        photos[ currentTargetIndex].vbo.drawElements(GL_TRIANGLES, photos[0].numIndices);
        //        photos[currentTargetIndex].tex.unbind();
    }
    
}

// this is called when your system is no longer drawing.
// Right after this selfUpdate() and selfDraw() won't be called any more
void CloudsVisualSystemPhotoGlitch::selfEnd(){
	
}

// this is called when you should clear all the memory and delet anything you made in setup
void CloudsVisualSystemPhotoGlitch::selfExit()
{
    clearSource();
    clearTarget();
    
    //    for(int i =0 ; i< photos.size(); i++){
    //        photos[i].clear();
    //    }
}

//events are called when the system is active
//Feel free to make things interactive for you, and for the user!
void CloudsVisualSystemPhotoGlitch::selfKeyPressed(int key)
{
    
}

void CloudsVisualSystemPhotoGlitch::selfKeyReleased(ofKeyEventArgs & args){
	
}

void CloudsVisualSystemPhotoGlitch::selfMouseDragged(int x, int y, int button){
    
}

void CloudsVisualSystemPhotoGlitch::selfMouseMoved(int x, int y, int button){
	
}

void CloudsVisualSystemPhotoGlitch::selfMousePressed(int x, int y, int button){
	
}

void CloudsVisualSystemPhotoGlitch::selfMouseReleased(int x, int y, int button){
	
}

void CloudsVisualSystemPhotoGlitch::shuffle()
{
    bool * slots = new bool[numCells];
    for (int i = 0; i < numCells; i++) {
        slots[i] = false;
    }
    
    int numOccupied = 0;
    int lastOpenSlotIdx = 0;
    for (int i = 0; i < numCells; i++) {
        int openSlotIdx = 0;
        if (numOccupied < numCells * 0.75f) {
            // find an open slot randomly
            do {
                openSlotIdx = ofRandom(numCells - 1);
            } while (slots[openSlotIdx] == true);
        }
        else {
            // find an open slot sequentially
            for (int j = lastOpenSlotIdx; j < numCells; j++) {
                if (slots[j] == false) {
                    // boom, found one
                    openSlotIdx = lastOpenSlotIdx = j;
                    break;
                }
            }
        }
        
        // get the row and col from the index
        int openCol = openSlotIdx % numDivCols;
        int openRow = openSlotIdx / numDivCols;
        
        // set the target position using the row and col indices
        cells[i].col = openCol;
        cells[i].row = openRow;
        
        tween(i, openSlotIdx);
        
        
        slots[openSlotIdx] = true;
        ++numOccupied;
    }
    
    delete [] slots;
}

void CloudsVisualSystemPhotoGlitch::sortHue()
{
    vector<int> slots;
    for (int i = 0; i < numCells; i++) {
        slots.push_back(i);
    }
    
    sort(slots.begin(), slots.end(), CloudsVisualSystemPhotoGlitch::sortIdxForHue);
    
    for (int i = 0; i < numCells; i++) {
        int slotIdx = slots[i];
        
        // set the target position using the row and col indices
        cells[i].col = slotIdx % numDivCols;
        cells[i].row = slotIdx / numDivCols;
        
        tween(i, slotIdx);
    }
}

void CloudsVisualSystemPhotoGlitch::sortBri()
{
    vector<int> slots;
    for (int i = 0; i < numCells; i++) {
        slots.push_back(i);
    }
    
    sort(slots.begin(), slots.end(), CloudsVisualSystemPhotoGlitch::sortIdxForBri);
    
    for (int i = 0; i < numCells; i++) {
        int slotIdx = slots[i];
        
        // set the target position using the row and col indices
        cells[i].col = slotIdx % numDivCols;
        cells[i].row = slotIdx / numDivCols;
        
        tween(i, slotIdx);
    }
}

void CloudsVisualSystemPhotoGlitch::sortTargetBrightness(){
    vector<int> sourceSlots;
    vector<int> targetSlots;
    vector  <int>  remainingIndices;
    vector<compareObj> sourceCompare, targetCompare;
    
    for (int i = 0; i < numCells; i++) {
        sourceSlots.push_back(i);
        targetSlots.push_back(i);
        remainingIndices.push_back(i);
        
        compareObj sourceObj, targetObj;
        
        sourceObj.cell = &cells[i];
        sourceObj.index = i;
        sourceCompare.push_back(sourceObj);
        
        targetObj.cell = &currentTarget->cells[i];
//        targetObj.cell = &photos[currentTargetIndex].cells[i];
        targetObj.index = i;
        targetCompare.push_back(targetObj);
    }
    
    sort(targetCompare.begin(), targetCompare.end(),sortIdxForBrightnessTargetNew);
    sort(sourceCompare.begin(), sourceCompare.end(),sortIdxForBrightnessTargetNew);
    
    random_shuffle(remainingIndices.begin(), remainingIndices.end());
    
    if(bRandomSort){
        for (int i = 0; i < numCells ; i++) {
          cells[sourceCompare[i].index].col = currentTarget->cells[targetCompare[i].index].col;
          cells[sourceCompare[i].index].row = currentTarget->cells[targetCompare[i].index].row;
          tweenTarget(sourceCompare[i].index,remainingIndices[i]);
        }
    }
    else{
        for (int i = 0; i < numCells ; i++) {
           cells[sourceCompare[i].index].col = currentTarget->cells[targetCompare[i].index].col;
           cells[sourceCompare[i].index].row = currentTarget->cells[targetCompare[i].index].row;
            tweenTarget(sourceCompare[i].index);
        }
    }
    
    
}

void CloudsVisualSystemPhotoGlitch::sortTarget(){
    vector<int> sourceSlots;
    vector<int> targetSlots;
    vector  <int>  remainingIndices;
    vector<compareObj> sourceCompare, targetCompare;
    
    for (int i = 0; i < numCells; i++) {
        sourceSlots.push_back(i);
        targetSlots.push_back(i);
        remainingIndices.push_back(i);
        
        compareObj sourceObj, targetObj;
        
        sourceObj.cell = &cells[i];
        sourceObj.index = i;
        sourceCompare.push_back(sourceObj);
        
        targetObj.cell = &currentTarget->cells[i];
        targetObj.index = i;
        targetCompare.push_back(targetObj);
        
        
    }
    
    sort(targetCompare.begin(), targetCompare.end(),sortIdxForHueTargetNew);
    sort(sourceCompare.begin(), sourceCompare.end(),sortIdxForHueTargetNew);
    
    if(bRandomSort){
        for (int i = 0; i < numCells ; i++) {
            
            cells[sourceCompare[i].index].col = currentTarget->cells[targetCompare[i].index].col;
            cells[sourceCompare[i].index].row = currentTarget->cells[targetCompare[i].index].row;
            tweenTarget(sourceCompare[i].index,remainingIndices[i]);
        }
    }
    else{
        for (int i = 0; i < numCells ; i++) {
           cells[sourceCompare[i].index].col = currentTarget->cells[targetCompare[i].index].col;
           cells[sourceCompare[i].index].row = currentTarget->cells[targetCompare[i].index].row;
            tweenTarget(sourceCompare[i].index);
        }
    }
}

void CloudsVisualSystemPhotoGlitch::reorder()
{
    for (int i = 0; i < numCells; i++) {
        cells[i].col = cells[i].origCol;
        cells[i].row = cells[i].origRow;
        tween(i);
    }
}

void CloudsVisualSystemPhotoGlitch::tweenAll()
{
    for (int i = 0; i < numCells; i++) {
        tween(i);
    }
}

void CloudsVisualSystemPhotoGlitch::tween(int i, int j)
{
    if (j == -1) j = i;
    
    int vertIdx = cells[i].idx * kVertsPerCell * kCoordsPerVert;
    
    if (bDoPerpendicular) {
        cells[i].tweenX.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 0], cells[i].col * screenSliceWidth,  tweenDuration / 2, tweenDelay * j);
        cells[i].tweenY.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 1], cells[i].row * screenSliceHeight, tweenDuration / 2, tweenDuration / 2 + tweenDelay * j);
    }
    else {
        cells[i].tweenX.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 0], cells[i].col * screenSliceWidth,  tweenDuration, tweenDelay * j);
        cells[i].tweenY.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 1], cells[i].row * screenSliceHeight, tweenDuration, tweenDelay * j);
    }
    
    cells[i].tweenX.start();
    cells[i].tweenY.start();
}

void CloudsVisualSystemPhotoGlitch::tweenTarget(int i, int j)
{
    if (j == -1) j = i;
    
    int vertIdx = cells[i].idx * kVertsPerCell * kCoordsPerVert;
    
    if (bDoPerpendicular) {
        cells[i].tweenX.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 0], cells[i].col * screenSliceWidth,  tweenDuration / 2, tweenDelay * j);
        cells[i].tweenY.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 1], cells[i].row * screenSliceHeight, tweenDuration / 2, tweenDuration / 2 + tweenDelay * j);
    }
    else {
        cells[i].tweenX.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 0], cells[i].col * screenSliceWidth,  tweenDuration, tweenDelay * j);
        cells[i].tweenY.setParameters(easing, ofxTween::easeOut, verts[vertIdx + 1], cells[i].row * screenSliceHeight, tweenDuration, tweenDelay * j);
    }
    
    cells[i].tweenX.start();
    cells[i].tweenY.start();
}

