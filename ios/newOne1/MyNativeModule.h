//
//  MyNativeModule.h
//  fxtf
//
//  Created by Tim Yuan on 17/3/2020.
//  Copyright © 2020 Facebook. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <React/RCTBridgeModule.h>
#import <React/RCTEventEmitter.h>
#import <UIKit/UIKit.h>

//#include "easywsclient.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface MyNativeModule : RCTEventEmitter<RCTBridgeModule> {

//easywsclient::ClientConnection g_conn;
  int flag;
}

@property (nonatomic, weak) UIView *textField; // 或 UITextView

-(void)sendNotification:(const char *)str;

-(void)sendConnection:(const char *)msg;

@end

NS_ASSUME_NONNULL_END
