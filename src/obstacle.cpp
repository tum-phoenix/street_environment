#include "street_environment/obstacle.h"

#include "lib/objectTracker/objectTracker.h"
#include "lib/objectTracker/objectTracker_emxAPI.h"

namespace street_environment{
Obstacle::Obstacle() : m_position(0, 0){

    for(int i = 0; i < 16; i++){
        stateCovariance[i] = 0;
    }
    stateCovariance[0] = 1;
    stateCovariance[5] = 1;
    stateCovariance[10] = 1;
    stateCovariance[15] = 1;
    m_init = true;
    fistRun = true;
}

bool Obstacle::validKalman() const{
    return m_validKalman;
}

bool Obstacle::match(const Obstacle &obj) const{
    if(!EnvironmentObject::match(obj)){
        return false;
    }
    //TODO use boundingBox
    if(validKalman()){
        //check if they are both on the same line
        if(lms::math::sgn(getStreetDistanceOrthogonal()) == lms::math::sgn(obj.getStreetDistanceOrthogonal())){
            if(fabs(getStreetDistanceTangential()-obj.getStreetDistanceTangential()) < 0.3){//TODO magic number
                return true;
            }
        }
    }else{
    //TODO use measurement accuracy
        return (position().distance(obj.position())<0.2);
    }
    return false;
}

lms::math::vertex2f Obstacle::position() const{
    return m_position;
}

lms::math::vertex2f Obstacle::viewDirection() const{
    return m_viewDirection;
}

void Obstacle::viewDirection(const lms::math::vertex2f &v){
    m_viewDirection = v;
}

float Obstacle::width(){
    return m_width;
}

void Obstacle::width(float w){
    m_width = w;
}

void Obstacle::updatePosition(const lms::math::vertex2f &position) {
    this->m_position = position;
    m_validKalman = false;
}

void Obstacle::simple(float distanceMoved){
    lms::math::vertex2f tmp = m_position;
    tmp = tmp.normalize()*distanceMoved;
    if(tmp.x > 0){
        tmp = tmp.negate();
    }
    m_position = m_position +tmp;
}

void Obstacle::kalman(const street_environment::RoadLane &middle, float distanceMoved){
    //simple(distanceMoved);

    static_assert(sizeof(state)/sizeof(double) == 4,"Obstacle::kalman: Size doesn't match idiot!");

    //Set old state
    for(uint i = 0; i < sizeof(state)/sizeof(double); i++){
        oldState[i] = state[i];
    }

    //get new values for kalman
    double trustModel = 0.001;
    double trustMeasure = 0.001;
    const emxArray_real_T *laneModel = emxCreate_real_T(middle.polarDarstellung.size(),1);
    for(uint i = 0; i < middle.polarDarstellung.size(); i++){
        laneModel->data[i] = middle.polarDarstellung[i];
    }
    //TODO
    float measureX =m_position.x;
    float measureY =m_position.y;
    //kalman it
    //1 for init
    if(fistRun)
        objectTracker(1,laneModel, middle.polarPartLength, state, stateCovariance, trustModel, trustMeasure,trustMeasure, measureX, measureY, distanceMoved,true);
    //else
    //    objectTracker(0,laneModel, middle.polarPartLength, state, stateCovariance, trustModel, trustMeasure,trustMeasure, measureX, measureY, distanceMoved,true);
    fistRun = false;

    //Convert the kalman-result
    arcLength = state[0];
    orthLength = state[2];
    //std::cout <<"KALMAN-vals: " <<"arcLength: "<<  arcLength << " orthLength: "<< orthLength<<std::endl;
    double currentLength = 0;

    for(int i = 1; i < (int)middle.points().size(); i++){
        double dd = middle.points()[i-1].distance(middle.points()[i]);
        if(currentLength +dd > arcLength){
            lms::math::vertex2f d = (middle.points()[i] -middle.points()[i-1]).normalize();
            //setzen der position
            //Bis jetzt nur tangential
            lms::math::vertex2f tang = d*(arcLength-currentLength);
            lms::math::vertex2f orth = d.rotateAntiClockwise90deg()*orthLength;
            this->m_position = middle.points()[i-1]+tang+orth;

            //Addiere den orthogonalen anteil
            break;
        }else{
            currentLength += dd;
        }
    }
    m_validKalman = true;
    m_init = false;
}

float Obstacle::getStreetDistanceOrthogonal() const{
    return orthLength;
}
float Obstacle::getStreetDistanceTangential() const{
    return arcLength;
}

} //street_environment
