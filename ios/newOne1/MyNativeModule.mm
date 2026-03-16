//
//  MyNativeModule.m
//  fxtf
//
//  Created by Tim Yuan on 17/3/2020.
//  Copyright © 2020 Facebook. All rights reserved.
//

#import "MyNativeModule.h"
#import <React/RCTLog.h>
#import <React/RCTBridge.h>
//#import "easywsclient.hpp"
#import <React/RCTUIManager.h>
#import <React/RCTView.h>
#import <React/RCTMultilineTextInputView.h>

#if RCT_DEV
#import <React/RCTDevLoadingView.h>
#endif

#include "md5/md5.h"
#include "key.h"
#include "json/json/json.h"
#include "TcpClient.h"
#include "functions.h"
//CTcpChatServer * g_tcpServer = NULL;
//CTcpClient * g_tcpClient = NULL;

//easywsclient::ClientConnection g_conn;
void * g_obj = NULL;

std::string g_deviceID = "";

@implementation MyNativeModule
{
  bool hasListeners;
  //RCTUIManager *_uiManager;
}

void onMessage(const char * msg)
{
    //printf(">>> %s\n", msg);
  if (g_obj != NULL) {
    [(__bridge MyNativeModule*)g_obj sendNotification:msg];
  }}

void send_heart_beat()
{
    char err[1024];
    //g_conn.send("goodbuy", 7, err);
  
}

void on_connect(int n)
{
    //printf("onConnect\n");
  if (g_obj != NULL) {
    [(__bridge MyNativeModule*)g_obj sendNotification:"onConnect"];
  }}

void on_close(int n)
{
    //printf("onClose\n");
  if (g_obj != NULL) {
    [(__bridge MyNativeModule*)g_obj sendNotification:"onClose"];
  }
}

void on_error(int n, const char * msg)
{
    //printf("onError\n");
}

// To export a module named MyNativeModule
RCT_EXPORT_MODULE(MyNativeModule);

RCT_EXPORT_METHOD(isOnIPad:(RCTResponseSenderBlock)callback)
{
  //RCTLogInfo(@"ios isOnIPad");
  
  
  //callback(@[[NSNull null],"testing1"]);
  // 因为是显示页面，所以让原生接口运行在主线程
    dispatch_async(dispatch_get_main_queue(), ^{
      
      //g_obj = (__bridge void *)self;
      if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone &&
          [[[UIDevice currentDevice] model] hasPrefix:@"iPad"]) {
        //NSLog(@"%%%%%%%%%%%% iphone app run on ipad");
        if (callback) {
            callback(@[@true]);
        }
      }
      else {
        if (callback) {
            callback(@[@false]);
        }
      }
      
    });
  
}

/*
RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(
  echoString:(NSString *)input
) {
  return input;
}
 */

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(
  getStatusBarHeight
) {
  if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone &&
          [[[UIDevice currentDevice] model] hasPrefix:@"iPad"]) {
        //NSLog(@"%%%%%%%%%%%% iphone app run on ipad");
        return @(0);
      }

  if ( @available(iOS 13.0, *)) {
        NSSet *set = [UIApplication sharedApplication].connectedScenes;
        UIWindowScene *windowScene = [set anyObject];
        UIStatusBarManager *statusBarManager = windowScene.statusBarManager;
        CGFloat statusBarHeight = statusBarManager.statusBarFrame.size.height;
        if( @available(iOS 16.0, *)) {
            BOOL needAdjust = (statusBarHeight == 44);
            if(windowScene.windows.firstObject.safeAreaInsets.top == 59 && needAdjust) {
                statusBarHeight = 59;
            }
        }
        return @(statusBarHeight);
    }
  if ( @available(iOS 11.0, *)) {
    CGFloat statusBarHeight = [UIApplication sharedApplication].statusBarFrame.size.height;
    CGFloat safeAreaTop = UIApplication.sharedApplication.windows.firstObject.safeAreaInsets.top;
    return @(MAX(statusBarHeight, safeAreaTop));
  }
  CGRect rect = [[UIApplication sharedApplication] statusBarFrame];
  //NSLog(@"%%%%%%%%%%%% iphone statusbar height %f",rect.size.height);
  return @(rect.size.height);
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(
  getBottomPadding
) {
  /*
  if (@available(iOS 11.0, *)) {
      UIWindow *window = UIApplication.sharedApplication.keyWindow;
      //CGFloat topPadding = window.safeAreaInsets.top;
      //CGFloat bottomPadding = window.safeAreaInsets.bottom;
      //NSLog(@"%%%%%%%%%%%% iphone topPadding %f, bottomPadding %f",topPadding, bottomPadding);
      return @(window.safeAreaInsets.bottom);
  }
  return @(0);
  */
  CGFloat bottomSafeAreaHeight = 0.0;
      
  if (@available(iOS 11.0, *)) {
      UIWindow *keyWindow = UIApplication.sharedApplication.keyWindow;
      if (keyWindow) {
          bottomSafeAreaHeight = keyWindow.safeAreaInsets.bottom;
      }
  }
  return @(bottomSafeAreaHeight);
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(
  isIOS16
                                       ) {
  if (@available(iOS 16.0, *)) {
    return @(1);
  }
  return @(0);
}

//  �����ṩ���з���,Callback
RCT_EXPORT_METHOD(getAppVersion:(RCTResponseSenderBlock)callback)
{
  RCTLogInfo(@"ios getAppVerison");
  
  
  //callback(@[[NSNull null],"testing1"]);
  // 因为是显示页面，所以让原生接口运行在主线程
    dispatch_async(dispatch_get_main_queue(), ^{
      
      g_obj = (__bridge void *)self;
      
      //g_conn.setEvents(on_connect, on_close, on_error, onMessage, send_heart_beat);
      char err[1024] = "";
      //g_conn.connectUrl("ws://demos.kaazing.com/echo", err);
      //g_conn.connectUrl("wss://259cxha06d.execute-api.ap-northeast-1.amazonaws.com/dev", err);

      // 在这里可以写需要原生处理的UI或者逻辑
        //NSLog(@"params = %@", params);
        if (callback) {
            callback(@[@"testing2"]);
        }
      [self sendNotification:"testIOSEvent1"];
    });
  
}

-(void)sendConnection:(const char *)msg {
  char err[1024];
  //g_conn.send(msg, strlen(msg), err);
}

- (NSArray<NSString *> *) supportedEvents
{
  return @[@"iosEvent"];
}

-(void)startObserving {
  hasListeners = YES;
}

-(void)stopObserving {
  hasListeners = NO;
}

- (void)sendNotification:(const char *)str {
  if (hasListeners)   {
    [self sendEventWithName:@"iosEvent" body:@{@"msg": [NSString stringWithFormat:@"%s", str],@"cmd":@"native"}];
  }
}

std::string testWriteFile1(char * path){
  std::string file = path;
  file += "/test.txt";
  FILE * fp = fopen(file.c_str(), "w");
  std::string ret = "write file ok";
  if (fp == NULL) {
    ret = "write file fail";
  }
  else {
    fclose(fp);
  }
  return ret;
}

RCT_REMAP_METHOD(init,
                 resolver:(RCTPromiseResolveBlock)resolve
                 rejecter:(RCTPromiseRejectBlock)reject)
{
  g_obj = (__bridge void *)self;
  //NSArray *events = ...
  //if (events) {
  if (true) {
    NSString * value = @"ok";
    NSString * homeDir = NSHomeDirectory();
    NSArray * array = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString * cachePath = array[0];
    std::string tst = testWriteFile1((char*)[cachePath UTF8String]);
    func_init();
    resolve(@{@"ret":value, @"homeDir":homeDir, @"cachePath":cachePath, @"tst":[NSString stringWithFormat:@"%s", tst.c_str()]});
  } else {
    NSError *err = [NSError errorWithDomain:@"error" code:(NSInteger)0 userInfo:nil];
    reject(@"0", @"error", err);
  }
}

void MD5Pwd1(const char * _in, char * pwd)
{
	char tmp[128];
	//sprintf(tmp,"qeWS?%s@34fFN", _in);
    //sprintf(tmp,"987p:Dw%sM#125", _in);
    sprintf(tmp,MD5_LOGIN_KEY, _in);
#ifndef _NEW_MD5_LIB
	CountMD5((unsigned char *)tmp, strlen(tmp), pwd);
#else
    MD5 md5 = MD5(tmp);
    std::string md5Result = md5.hexdigest();
    strcpy(pwd, md5Result.c_str());
#endif
}

RCT_REMAP_METHOD(MD5Pwd1,
                 protocol:(NSString *)value
                 resolver_MD5Pwd1:(RCTPromiseResolveBlock)resolve
                 rejecter_MD5Pwd1:(RCTPromiseRejectBlock)reject)
{
  
  const char* temp1 = std::string([value UTF8String]).c_str();
  
  char _pwd[128];
  MD5Pwd1(temp1, _pwd);
  resolve( [NSString stringWithFormat:@"%s", _pwd] );
}

/*
RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(
  jsonCmd:(NSString *)cmd
) {
  //const char* temp = std::string([cmd UTF8String]).c_str();
  const char *temp = [cmd cStringUsingEncoding:[NSString defaultCStringEncoding]];

  Json::Reader reader;
  Json::Value root;
  reader.parse(temp, root);
 
  Json::Value vCmd = root["cmd"];
  if (vCmd.asString() == "GetLocalIPAddress") {
    return [NSString stringWithFormat:@"%s", GetLocalIPAddress().c_str()];
  }
  else if (vCmd.asString() == "createTcpServer") {
    g_tcpServer = new CTcpChatServer();
    g_tcpServer->setEvents(onTcpServerEvent);
    std::string err = "";
    g_tcpServer->Start(5001, 3, err);
    return [NSString stringWithFormat:@"%s", err.c_str()];
  }
  else if (vCmd.asString() == "connectTcpServer") {
    g_tcpClient = new CTcpClient();
    std::string err = "";
    Json::Value vIp = root["ip"];
    Json::Value vPort = root["port"];
    g_tcpClient->Connect(vIp.asString().c_str(), vPort.asInt(), err);
    return [NSString stringWithFormat:@"%s", err.c_str()];
  }
  
  std::string err = "unknownCmd";
  
  return [NSString stringWithFormat:@"%s", err.c_str()];
}
*/

RCT_EXPORT_METHOD(setTextInput:(nonnull NSNumber *)reactTag) {
  dispatch_async(dispatch_get_main_queue(), ^{
    RCTUIManager *uiManager = (RCTUIManager *)[self bridge].uiManager;
    UIView *view = [uiManager viewForReactTag:reactTag];

    NSLog(@"View class: %@", NSStringFromClass([view class]));

    if ([view isKindOfClass:[UITextField class]] ||
        [view isKindOfClass:[UITextView class]] ||
        [view isKindOfClass:[RCTMultilineTextInputView class]]) {
      self.textField = view;
    } else {
      NSLog(@"The view is not a UITextField, UITextView, or RCTMultilineTextInputView.");
    }
  });
}


RCT_EXPORT_METHOD(switchToEmojiKeyboard) {
  dispatch_async(dispatch_get_main_queue(), ^{
    if ([self.textField isKindOfClass:[UITextField class]]) {
      [((UITextField *)self.textField) becomeFirstResponder];
      // 手动切换到 emoji 键盘
      [((UITextField *)self.textField) setKeyboardType:UIKeyboardTypeDefault];
    } else if ([self.textField isKindOfClass:[RCTMultilineTextInputView class]]) {
      [((RCTMultilineTextInputView *)self.textField) becomeFirstResponder];
      // 这里也可以设置键盘类型
      [((RCTMultilineTextInputView *)self.textField) setKeyboardType:UIKeyboardTypeDefault];
    }
  });
}


RCT_REMAP_METHOD(jsonCmd,
                 protocol:(NSString *)cmd
                 resolver_jsonCmd:(RCTPromiseResolveBlock)resolve
                 rejecter_jsonCmd:(RCTPromiseRejectBlock)reject
) {
  //const char* temp = std::string([cmd UTF8String]).c_str();
  //const char *temp = [cmd cStringUsingEncoding:[NSString defaultCStringEncoding]];
  const char *temp = [cmd UTF8String];

  Json::Reader reader;
  Json::Value root;
  reader.parse(temp, root);
 
  Json::Value vCmd = root["cmd"];
  if (vCmd.asString() == "GetLocalIPAddress") {
      //resolve( [NSString stringWithFormat:@"%s", GetLocalIPAddress().c_str()] );
    resolve( [[NSString alloc] initWithUTF8String:GetLocalIPAddress().c_str()] );
    return;
  }
  else if (vCmd.asString() == "createGroup") {
      //resolve( [NSString stringWithFormat:@"%s", func_createGroup1(root, true).c_str()] );
    resolve( [[NSString alloc] initWithUTF8String:func_createGroup1(root, true, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "loadGroup") {
      //resolve( [NSString stringWithFormat:@"%s", func_createGroup1(root, false).c_str()] );
    resolve( [[NSString alloc] initWithUTF8String:func_createGroup1(root, false, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "deleteGroup") {
      //resolve( [NSString stringWithFormat:@"%s", func_deleteGroup1(root).c_str()] );
    resolve( [[NSString alloc] initWithUTF8String:func_deleteGroup1(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "startGroup") {
    //resolve( [NSString stringWithFormat:@"%s", func_startGroup(root, onMessage).c_str()]);
    resolve( [[NSString alloc] initWithUTF8String:func_startGroup(root, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "stopGroup") {
    //resolve( [NSString stringWithFormat:@"%s", func_stopGroup(root, onMessage).c_str()]);
    resolve( [[NSString alloc] initWithUTF8String:func_stopGroup(root, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "sendMsg") {
    //resolve( [NSString stringWithFormat:@"%s", func_sendMsg(root, onMessage).c_str()]);
    resolve( [[NSString alloc] initWithUTF8String:func_sendMsg(root, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "encryptGroupAddress") {
    resolve( [NSString stringWithFormat:@"%s", func_encryptGroupAddress(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "decryptGroupAddress") {
    //resolve( [NSString stringWithFormat:@"%s", func_decryptGroupAddress(root).c_str()] );
    resolve( [[NSString alloc] initWithUTF8String:func_decryptGroupAddress(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "joinGroup") {
    //resolve( [NSString stringWithFormat:@"%s", func_joinGroup1(root, onMessage).c_str()]);
    resolve( [[NSString alloc] initWithUTF8String:func_joinGroup1(root, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "getGroupSeqs") {
    //resolve( [NSString stringWithFormat:@"%s", func_getGroupSeqs(root).c_str()]);
    resolve( [[NSString alloc] initWithUTF8String:func_getGroupSeqs(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "loadMsg") {
    //resolve( [NSString stringWithFormat:@"%s", func_loadMsg(root).c_str()]);
    resolve( [[NSString alloc] initWithUTF8String:func_loadMsg(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "loadMmember") {
    //resolve( [NSString stringWithFormat:@"%s", func_loadMember(root).c_str()]);
    resolve( [[NSString alloc] initWithUTF8String:func_loadMember(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "testing") {
    resolve( [[NSString alloc] initWithUTF8String:"OK"] );
    return;
  }
  else if (vCmd.asString() == "uploadFile") {
    //NSString * dir = NSTemporaryDirectory();
    //setTempDir([dir UTF8String]);
    resolve( [[NSString alloc] initWithUTF8String:func_uploadFile(root, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "downloadFile") {
    resolve( [[NSString alloc] initWithUTF8String:func_downloadFile(root, onMessage).c_str()] );
    return;
  }
  else if (vCmd.asString() == "md5Encrypt") {
    resolve( [[NSString alloc] initWithUTF8String:func_md5Encrypt(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "md5Verify") {
    resolve( [[NSString alloc] initWithUTF8String:func_md5Verify(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "webEncrypt") {
    resolve( [[NSString alloc] initWithUTF8String:func_webEncrypt(root).c_str()] );
    return;
  }
  else if (vCmd.asString() == "setDeviceID") {
    g_deviceID = root["deviceID"].asString();
    resolve( [[NSString alloc] initWithUTF8String:""] );
    return;
  }
  /*
  else if (vCmd.asString() == "createTcpServer") {
    g_tcpServer = new CTcpChatServer();
    g_tcpServer->setEvents(onTcpServerEvent);
    std::string err = "";
    g_tcpServer->Start(5001, 3, err);
    return [NSString stringWithFormat:@"%s", err.c_str()];
  }
  else if (vCmd.asString() == "connectTcpServer") {
    g_tcpClient = new CTcpClient();
    std::string err = "";
    Json::Value vIp = root["ip"];
    Json::Value vPort = root["port"];
    g_tcpClient->Connect(vIp.asString().c_str(), vPort.asInt(), err);
    return [NSString stringWithFormat:@"%s", err.c_str()];
  }
  */
  
  std::string err = "unknownCmd";
  
  resolve( [NSString stringWithFormat:@"%s", err.c_str()] );
}

@end
