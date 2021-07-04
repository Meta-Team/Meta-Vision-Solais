//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_INPUTSOURCE_H
#define META_VISION_SOLAIS_INPUTSOURCE_H

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <chrono>
#include "Parameters.pb.h"
#include "FrameCounterBase.h"
#include "Utilities.h"

namespace meta {

class InputSource : public FrameCounterBase {
public:

    virtual ~InputSource() {}

    // Open function can be implemented differently

    /**
     * Whether the streaming source is opened
     * @return
     */
    virtual bool isOpened() const = 0;

    /**
     * Close the streaming source
     */
    virtual void close() = 0;

    /**
     * Get current frame capture time. 0 indicates the end of the stream.
     * @return
     */
    virtual TimePoint getFrameCaptureTime() const = 0;

    /**
     * Get current frame. No need for data copying due to double buffering.
     * @return
     */
    virtual const cv::Mat &getFrame() const = 0;

    /**
     * Fetch next frame. Called after current frame is accepted (no copy needed due to double buffering).
     */
    virtual void fetchNextFrame() {};

};

}

#endif //META_VISION_SOLAIS_INPUTSOURCE_H
