/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#pragma once
#ifndef LIBREALSENSE_STREAM_H
#define LIBREALSENSE_STREAM_H

#include "types.h"

namespace rsimpl
{
    struct stream_interface
    {
        rs_extrinsics                           get_extrinsics_to(const stream_interface & r) const;
        virtual rsimpl::pose                    get_pose() const = 0;
        virtual float                           get_depth_scale() const = 0;
        virtual int                             get_mode_count() const { return 0; }
        virtual const stream_mode &             get_mode(int mode) const { throw std::logic_error("no modes"); }

        virtual bool                            is_enabled() const = 0;
        virtual rs_intrinsics                   get_intrinsics() const = 0;
        virtual rs_intrinsics                   get_rectified_intrinsics() const = 0;
        virtual rs_format                       get_format() const = 0;
        virtual int                             get_framerate() const = 0;

        virtual int                             get_frame_number() const = 0;
        virtual const byte *                    get_frame_data() const = 0;    
    };

    struct native_stream final : public stream_interface
    {
        const device_config &                   config;
        const rs_stream                         stream;
        std::vector<stream_mode>                modes;
        std::shared_ptr<stream_buffer>          buffer;

                                                native_stream(device_config & config, rs_stream stream);

        pose                                    get_pose() const override { return config.info.stream_poses[stream]; }
        float                                   get_depth_scale() const override { return config.depth_scale; }
        int                                     get_mode_count() const override { return (int)modes.size(); }
        const stream_mode &                     get_mode(int mode) const override { return modes[mode]; }

        bool                                    is_enabled() const override { return buffer || config.requests[stream].enabled; }
        subdevice_mode_selection                get_mode() const;
        rs_intrinsics                           get_intrinsics() const override { return config.intrinsics.get(get_mode().get_intrinsics_index(stream)); }
        rs_intrinsics                           get_rectified_intrinsics() const override { return config.intrinsics.get_rect(get_mode().get_intrinsics_index(stream)); }
        rs_format                               get_format() const override { return get_mode().get_format(stream); }
        int                                     get_framerate() const override { return get_mode().get_framerate(stream); }

        int                                     get_frame_number() const override { return buffer->get_front_number(); }
        const byte *                            get_frame_data() const override { return buffer->get_front_data(); }
    };

    class rectified_stream final : public stream_interface
    {
        const stream_interface &                source;
        mutable std::vector<int>                table;
        mutable std::vector<byte>               image;
        mutable int                             number;
    public:
                                                rectified_stream(const stream_interface & source) : source(source), number() {}

        pose                                    get_pose() const override { return {{{1,0,0},{0,1,0},{0,0,1}}, source.get_pose().position}; }
        float                                   get_depth_scale() const override { return source.get_depth_scale(); }

        bool                                    is_enabled() const override { return source.is_enabled(); }
        rs_intrinsics                           get_intrinsics() const override { return source.get_rectified_intrinsics(); }
        rs_intrinsics                           get_rectified_intrinsics() const override { return source.get_rectified_intrinsics(); }
        rs_format                               get_format() const override { return source.get_format(); }
        int                                     get_framerate() const override { return source.get_framerate(); }

        int                                     get_frame_number() const override { return source.get_frame_number(); }
        const byte *                            get_frame_data() const override;
    };

    class aligned_stream final : public stream_interface
    {
        const stream_interface &                from, & to;
        mutable std::vector<byte>               image;
        mutable int                             number;
    public:
                                                aligned_stream(const stream_interface & from, const stream_interface & to) : from(from), to(to), number() {}

        pose                                    get_pose() const override { return to.get_pose(); }
        float                                   get_depth_scale() const override { return to.get_depth_scale(); }

        bool                                    is_enabled() const override { return from.is_enabled() && to.is_enabled(); }
        rs_intrinsics                           get_intrinsics() const override { return to.get_intrinsics(); }
        rs_intrinsics                           get_rectified_intrinsics() const override { return to.get_rectified_intrinsics(); }
        rs_format                               get_format() const override { return from.get_format(); }
        int                                     get_framerate() const override { return from.get_framerate(); }

        int                                     get_frame_number() const override { return from.get_frame_number(); }
        const byte *                            get_frame_data() const override;
    };
}

#endif