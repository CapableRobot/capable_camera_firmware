// Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

const std = @import("std");
const fs = std.fs;
const mem = std.mem;
const print = std.debug.print;

const web = @import("zhp");

const camParamBase = @import("../cfg/camParamBase.zig");
const imgCfg = @import("../cfg/mutImgCfg.zig");

const updateStr: []const u8 = 
\\Updated Imager Parameters:
\\Width  = {},
\\Height = {},
\\FPS    = {}
;

const failStr: []const u8 =
\\Failed to update!
;

pub fn validate(cfg_params: imgCfg.MutableImgCfg) bool{
    var isGood: bool = true;
    if (cfg_params.hpx == 0){ isGood = false; }
    if (cfg_params.vpx == 0){ isGood = false; }
    if (cfg_params.fps == 0){ isGood = false; }
    if (cfg_params.hpx > 4096){ isGood = false; }
    if (cfg_params.vpx > 2160){ isGood = false; }
    if (cfg_params.fps >   30){ isGood = false; }    
    
    return isGood;
}

pub const Handler = struct {
    
    pub fn post(self: *Handler, 
                request: *web.Request, 
                response: *web.Response) !void {
       try response.headers.append("Content-Type", "text/plain");
        
       var respBuff: [256]u8 = undefined;
       const outputSlice = respBuff[0..];
       var cfg_params = imgCfg.MutableImgCfg {.hpx = 0, 
                                              .vpx = 0,
                                              .fps = 0};
       var goodInput: bool = true;
     
       var content_type = request.headers.getDefault("Content-Type", "");
       if (std.mem.startsWith(u8, content_type, "application/json")) {
         if (!request.read_finished) {
           if (request.stream) |stream| {
             try request.readBody(stream);
           }
         }
         if (request.content) |content| {
           switch (content.type) {
             .TempFile => {
               return error.NotImplemented; 
               // TODO: Parsing should use a stream
             },
             .Buffer => {
               var reqContent = content.data.buffer;
               var contentStream = std.json.TokenStream.init(reqContent);
               cfg_params = try std.json.parse(imgCfg.MutableImgCfg,
                                               &contentStream, .{});
               goodInput = validate(cfg_params);
               if(goodInput){
                 try camParamBase.write_out_cam(camParamBase.fullFilePath, 
                                                cfg_params);
               }
             }
           }
         }           
       }
       if (goodInput){
           const outputStr = try std.fmt.bufPrint(outputSlice, updateStr, 
            .{cfg_params.hpx, cfg_params.vpx, cfg_params.fps});
           try response.stream.writeAll(outputStr);
       } else {
           const outputStr = try std.fmt.bufPrint(outputSlice, failStr, .{});
           try response.stream.writeAll(outputStr);       
       }
    }
    
};
