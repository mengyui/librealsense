//
//  RealSenseReader.swift
//  hello
//
//  Created by Yu on 2020/3/17.
//  Copyright © 2020. All rights reserved.
//

import SwiftUI
import CoreImage
import CoreGraphics

class RealSenseReaderContext : ObservableObject {
    
    @Published var depth: CGImage?
    @Published var ir: CGImage?
    @Published var color: CGImage?
    
    @Published var name: String?
    
    var ctx: OpaquePointer? = nil
    var pipe: OpaquePointer? = nil
    
    var color_queue: OpaquePointer? = nil
    var color_map: OpaquePointer? = nil
    
    public func start() {
        var e:OpaquePointer? = nil
        
        rs2_log_to_console(RS2_LOG_SEVERITY_ERROR, &e); check(err: e)
        
        let RS2_API_VERSION:Int32 = 23301
        ctx = rs2_create_context(RS2_API_VERSION, &e); check(err: e)
        
        let devices = rs2_query_devices(ctx, &e); check(err: e)
        
        let devices_count = rs2_get_device_count(devices, &e); check(err: e)
        print("devices count: \(devices_count)")
        if devices_count > 0 {
            let dev = rs2_create_device(devices, 0, &e); check(err: e)
            let cam_name = String(cString: rs2_get_device_info(dev, RS2_CAMERA_INFO_NAME, &e)); check(err: e)
            let cam_sn = String(cString: rs2_get_device_info(dev, RS2_CAMERA_INFO_SERIAL_NUMBER, &e)); check(err: e)
            rs2_delete_device(dev)
            name = cam_name + " SN: " + cam_sn
        }
                
        rs2_delete_device_list(devices)
        
        color_queue = rs2_create_frame_queue(1, &e); check(err: e)
        color_map = rs2_create_colorizer(&e); check(err: e)
        rs2_start_processing_queue(color_map, color_queue, &e); check(err: e)
        
        pipe = rs2_create_pipeline(ctx, &e); check(err: e)
        //let cfg = rs2_create_config(&e); check(err: e)
        //rs2_config_enable_stream(cfg, RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30, &e); check(err: e)
        //rs2_config_enable_stream(cfg, RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, &e); check(err: e)
        //let pp = rs2_pipeline_start_with_config(pipe, cfg, &e); check(err: e)
        let pp = rs2_pipeline_start(pipe, &e); check(err: e)
        rs2_delete_pipeline_profile(pp)
    }
    
    public func stop() {
        var e:OpaquePointer? = nil

        rs2_pipeline_stop(pipe, &e); check(err: e)
        rs2_delete_pipeline(pipe)
        
        rs2_delete_processing_block(color_map)
        rs2_delete_frame_queue(color_queue)
        
        rs2_delete_context(ctx)
    }
    
    public func grab() {
        var e:OpaquePointer? = nil

        let frames = rs2_pipeline_wait_for_frames(pipe, 5000, &e); check(err: e)
        let is_composite_frame = 0 != rs2_is_frame_extendable_to(frames, RS2_EXTENSION_COMPOSITE_FRAME, &e); check(err: e)
        if is_composite_frame {
            let embedded_fraems_count = rs2_embedded_frames_count(frames, &e); check(err: e)
            for i in 0...embedded_fraems_count - 1 {
                let frame = rs2_extract_frame(frames, i, &e); check(err: e)
                let profile = rs2_get_frame_stream_profile(frame, &e); check(err: e)
                switch getStreamType(mode: profile) {
                case RS2_STREAM_DEPTH:
                    rs2_process_frame(color_map, frame, &e); check(err: e)
                    let colorized = rs2_wait_for_frame(color_queue, 5000, &e); check(err: e)
                    self.depth = convertToCGImage(frame: colorized)
                    rs2_release_frame(colorized)
                    break
                case RS2_STREAM_COLOR:
                    self.color = convertToCGImage(frame: frame)
                    break
                case RS2_STREAM_INFRARED:
                    self.ir = convertToCGImage(frame: frame)
                    break
                default:
                    break
                }
                rs2_release_frame(frame)
            }
        }
        rs2_release_frame(frames)
    }
    
    private func check(err: OpaquePointer?) {
        if let err = err {
            let f = String(cString: rs2_get_failed_function(err))
            let m = String(cString: rs2_get_error_message(err))
            print("ERROR: Function: \(f) DESC: \(m)")
        }
    }
    
    private func getResolution(mode: OpaquePointer!) -> (Int, Int) {
        var e:OpaquePointer? = nil

        var width:Int32 = 0
        var height:Int32 = 0
        rs2_get_video_stream_resolution(mode, &width, &height, &e); check(err: e)
        return (Int(width), Int(height))
    }
    
    private func getStreamType(mode: OpaquePointer!) -> rs2_stream {
        var e:OpaquePointer? = nil
        var stream = RS2_STREAM_ANY
        var format = RS2_FORMAT_ANY
        var index = Int32(0)
        var uid = Int32(0)
        var fps = Int32(0)
        rs2_get_stream_profile_data(mode, &stream, &format, &index, &uid, &fps, &e); check(err: e)
        return stream
    }
    
    private func convertToCGImage(frame: OpaquePointer!) -> CGImage? {
        var e:OpaquePointer? = nil
        
        //let (width, height) = getResolution(mode: frame)
        let width = Int(rs2_get_frame_width(frame, &e)); check(err: e)
        let height = Int(rs2_get_frame_height(frame, &e)); check(err: e)
        let data = rs2_get_frame_data(frame, &e); check(err: e)
        let size = rs2_get_frame_data_size(frame, &e); check(err: e)

        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo = CGBitmapInfo(rawValue: CGImageAlphaInfo.none.rawValue | CGImageByteOrderInfo.orderDefault.rawValue)
        
        let dataProvider = CGDataProvider(dataInfo: nil, data: data!, size: Int(size), releaseData: {_,_,_ in })
        let image = CGImage(width: width, height: height, bitsPerComponent: 8, bitsPerPixel: 24, bytesPerRow: width * 24 / 8, space: colorSpace, bitmapInfo: bitmapInfo, provider: dataProvider!, decode: nil, shouldInterpolate: false, intent: .defaultIntent)
        return image
    }
}
