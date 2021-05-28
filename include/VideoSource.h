//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_VIDEOSOURCE_H
#define META_VISION_SOLAIS_VIDEOSOURCE_H

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "Parameters.pb.h"
#include "FrameCounterBase.h"

namespace meta {

class VideoSource : public FrameCounterBase {
public:

    virtual ~VideoSource() {}

    /**
     * Start streaming and fetch the first frame.
     * @param params
     * @return
     */
    virtual bool open(const package::ParamSet &params) = 0;

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
     * Get current frame ID. -1 indicates the end of the stream.
     * @return
     */
    virtual int getFrameID() const = 0;

    /**
     * Get current frame. No need for data copying due to double buffering.
     * @return
     */
    virtual const cv::Mat &getFrame() const = 0;

    /**
     * Fetch next frame. Called after current frame is accepted (no copy needed due to double buffering).
     */
    virtual void fetchNextFrame() {};

protected:

    static constexpr int FRAME_ID_MAX = 0x7FFF;

};

}

#endif //META_VISION_SOLAIS_VIDEOSOURCE_H
