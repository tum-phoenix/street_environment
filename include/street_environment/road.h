#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <vector>
#include "lms/inheritance.h"
#include "lms/math/vertex.h"
#include "lms/math/polyline.h"
#include "street_environment/street_environment.h"


#ifdef USE_CEREAL
#include "lms/serializable.h"
#include "cereal/cerealizable.h"
#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/base_class.hpp"
#endif

namespace street_environment {

    enum class RoadLaneType {
        LEFT, MIDDLE, RIGHT
    };

    class RoadLane : public EnvironmentObject, public lms::Inheritance, public lms::math::polyLine2f{
        RoadLaneType m_type;
    public:
        bool isSubType(size_t hashcode) const override{
            std::cout << "########################################WTH THIS IS CALLED????"<<std::endl;
            return lms::Impl<EnvironmentObject,lms::math::polyLine2f>::isSubType(hashcode,this);
        }

        RoadLane(){}

        virtual bool match(const RoadLane &obj) const{
            //doesn't handle subclasses
            if(!EnvironmentObject::match(obj)){
                return false;
            }
            //TODO

            return false;

        }
        virtual ~RoadLane() {}

        int getType()const override{
           return 0;
        }

        /**
         * @brief polarDarstellung
         * polarDarstellung[0] is the y-deviance
         * polarDarstellung[1] is the start-angle in rad
         * polarDarstellung[>1] Krümmungen relativ zum vorherigen stück
         */
        std::vector<double> polarDarstellung; //TODO english name :)
        float polarPartLength;
        RoadLaneType type() const{
            return m_type;
        }

        void type(RoadLaneType type){
            m_type = type;
        }
    };


    typedef std::shared_ptr<RoadLane> RoadLanePtr;
}  // namespace street_environment

#ifdef USE_CEREAL


template <class Archive>
void serialize( Archive & archive) {
    archive (cereal::base_class<street_environment::EnvironmentObject>(this),
                cereal::base_class<lms::math::polyLine2f>(this),
              m_type, polarDarstellung, polarPartLength);
}

namespace cereal {

template <class Archive>
struct specialize<Archive, street_environment::RoadLane, cereal::specialization::member_serialize> {};
  // cereal no longer has any ambiguity when serializing street_environment::RoadLane

}  // namespace cereal


//CEREAL_REGISTER_TYPE(street_environment::RoadLane)
//CEREAL_REGISTER_DYNAMIC_INIT(street_environment)
#endif

#endif /* ENVIRONMENT_H */
