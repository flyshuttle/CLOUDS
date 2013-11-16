//
//  gCell.cpp
//  Colony
//
//  Created by Omer Shapira on 10/11/13.
//
//

#include "gCell.h"

colonyCell::colonyCell(const ofPoint initialPosition) //As illegal default parameter
{
    if (!isInsideBoard(initialPosition)){
        position = ofPoint(ofRandom(ofGetWidth()),ofGetHeight());
    } else {
        position = ofPoint(initialPosition);
    }
    
    /* Default Params */
    //TODO Replace magic numbers
    acceleration = ofVec2f(0,0);
    velocity = ofVec2f(ofRandom(-1,1), ofRandom(-1,1));
    cellSize = 1;
    age = 0;
    nutrientLevel = 50;
    deathThreshold = .001;
    maxSpeed = .4;
    maxForce = .4;
    maxSize = ofRandom(15, 25);
    lifespan = ofRandom(90, 600);
    fertile = ofRandomuf() > .5;
    dead = false;
    hasReplicated = false;
    
    //Private stuff
    separationDist = 20.0;
    alignmentDist = 50.0;
}

void colonyCell::update()
{
    //forces
    velocity += acceleration;
    velocity.limit(maxSpeed);
    position += velocity;
    
    //housekeeping
    acceleration *= 0; //TODO: Why, actually?
    if (lastFeedValue > nutrientLevel && cellSize <= maxSize){ cellSize += (lastFeedValue/2500.0); }
    if (lastFeedValue < nutrientLevel){ cellSize -= .01; }
    if (age > lifespan || hasReplicated){ cellSize -= .03;}
    if (cellSize <= deathThreshold){ dead = true; }
    age++;
}

void colonyCell::draw()
{
    ofPushStyle();
    ofCircle(position.x, position.y, cellSize); //TODO: This is where you do art
    ofPopStyle();
}

void colonyCell::doApplyForce(ofPoint _force)
{
    //TODO: Add mass
    acceleration += _force;
}

void colonyCell::doApplyFlock(neighbor_iterator iter){
    
    ofPoint separate,align,cohere;
    float ss = separationDist * separationDist;
    float aa = alignmentDist  * alignmentDist;
    float count = 0;
    while (iter.hasNext()) {
        ofPoint diff = position - ((*iter)->getPosition());
        float dd = diff.lengthSquared();
        if ((dd > 0) && (dd < ss)) {
            separate += diff/dd;
        }
        if ((dd > 0) && (dd < aa)){
            align  += iter->get()->velocity;
            cohere += iter->get()->position;
        }
        count++;
        iter++;
    }
    cohere /= count;
    align /=  count;
    //TODO: Remove magic numbers
    ofVec3f steer = (   separate.normalized()   * 50
                     +  cohere.normalized()     * 0.07
                     +  align.normalized()      * 0.033
                     ) * maxSpeed - velocity;
    steer.limit(maxSpeed);
    
    doApplyForce(steer);
}

void colonyCell::doFeedCellWidth(ofPixels &_pixels){
    int safeX = ofClamp(position.x,0,ofGetWidth()-1);
	int safeY = ofClamp(position.y,0,ofGetHeight()-1);
    lastFeedValue = _pixels.getColor(safeX, safeY).getBrightness();
}


void colonyCell::doApplyBorders()
{
    //FIXME: Oh oh FIXME, FIXME if you think that
    //       you've leared any math before
    
    //TODO: Remove magic numbers
    
    if (position.x >= ofGetWidth() + 30) {
        float diff = position.x - ofGetWidth();
        velocity.x = velocity.x * -1*diff;
    }
    
    if (position.x <= -30) {
        float diff = position.x - 0;
        velocity.x = velocity.x * -1*diff;
    }
    
    if (position.y >= ofGetHeight() +30) {
        float diff = position.y - ofGetHeight();
        velocity.y = velocity.y * -1*diff;
    }
    
    if (position.y <= - 30) {
        float diff = position.y - 0;
        velocity.y = velocity.y * -1*diff;
    }

}

const ofPoint& colonyCell::getPosition() const
{
    return position;
}

cellPtr colonyCell::doGetReplicated()
{
    hasReplicated = true;
    cellSize *= 0.5; //TODO: Check if this really need to shrink to 0.5
    return cellPtr(new colonyCell(getPosition()));
}

bool colonyCell::isInsideBoard(ofPoint p){ return (ofInRange(p.x, 0, ofGetWidth()) && ofInRange(p.y, 0, ofGetHeight())); }
bool colonyCell::isFertile(){ return fertile; }
bool colonyCell::isReadyToReplicate(){ return (lastFeedValue > nutrientLevel) && (age > fertilityAge); }
bool colonyCell::isDead(){ return dead; }
void colonyCell::doSetPosition(const ofPoint& p){ position = p; }