//
//  VerifyItem.swift
//  CIE ID
//
//  Created by Pierluigi De Gregorio on 18/02/21.
//  Copyright © 2021 IPZS. All rights reserved.
//

import Foundation

@objc
class VerifyItem : NSObject{
     
    private (set) var value : String
    private (set) var img : NSImage
    @objc
    var enlarge = false
    
    @objc
    init(image: NSImage, value : String)
    {
        
        self.value = value
        self.img = image
    }
    
    /*
    @objc
    func setEnlarge(_ flag : Bool)
    {
        self.enlarge = flag
    }
   */
}
