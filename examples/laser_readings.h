#ifndef DFKI_LASER_READINGS_H__
#define DFKI_LASER_READINGS_H__
#include <stdint.h>
#include <vector>

namespace DFKI {
    struct LaserReadings {
        /** The timestamp of this reading. The timestamp is the time at which the
         * laser passed the zero step (i.e. the step at the back of the device
         */
        uint64_t stamp;

        /** The step at which the range readings start. Step zero is at the back of
         * the device and turns counter-clockwise. There is \c resolution steps per
         * turn 
         */
        float min;

        /** How much steps there is per turn
         */
        uint32_t resolution;

        /** The rotation speed in microseconds per step
         */
        uint32_t speed;

        /** The ranges themselves: the distance to obstacles in millimeters
         */
        std::vector<uint32_t> ranges;
    };
}

#endif
