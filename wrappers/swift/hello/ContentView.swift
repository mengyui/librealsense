//
//  ContentView.swift
//  Yu
//
//  Created by Yu on 2020/3/17.
//  Copyright © 2020. All rights reserved.
//

import SwiftUI

struct ContentView: View {
    
    @EnvironmentObject var realsense_ctx: RealSenseReaderContext
    
    var body: some View {
        VStack {
            // Buttons and Info
            HStack {
                Button(action: {
                        self.realsense_ctx.start()
                    }, label: {
                        Text("Start")
                    })
                Button(action: {
                        self.realsense_ctx.stop()
                    }, label: {
                        Text("Stop")
                    })
                Button(action: {
                        self.realsense_ctx.grab()
                    }, label: {
                        Text("Grab")
                    })
                ShowCameraName(name: self.realsense_ctx.name)
            }
            // Images
            HStack {
                VStack {
                    Text("Depth")
                    ShowImage(image: self.realsense_ctx.depth).frame(minWidth:180, maxWidth: .infinity, minHeight:120, maxHeight: .infinity)
                }
                VStack {
                    Text("Color")
                    ShowImage(image: self.realsense_ctx.color).frame(minWidth:180, maxWidth: .infinity, minHeight:120, maxHeight: .infinity)
                }
            }
        }
    }
    
    private func ShowCameraName(name: String?) -> Text {
        if let camera_name = name {
            return Text(camera_name)
        }
        return Text("No camera.")
    }
    
    private func ShowImage(image: CGImage?) -> Image {
        if let cgImage = image {
            return Image(cgImage, scale: 2, label: Text("image"))
        }
        return Image("")
    }
}


struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
