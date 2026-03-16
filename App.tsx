/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 */

import React, { Component }  from 'react';
import type {PropsWithChildren} from 'react';
import {
  SafeAreaView,
  ScrollView,
  StatusBar, Platform, LogBox, AppState, DeviceEventEmitter, 
  StyleSheet,
  Text,
  useColorScheme,
  View,
  Alert,
  Animated,
  Dimensions,
  NativeModules,
} from 'react-native';

import {SafeAreaView as SafeAreaView1, SafeAreaInsetsContext   } from 'react-native-safe-area-context';

import NetInfo from '@react-native-community/netinfo';

import mobileAds, { BannerAd, BannerAdSize, TestIds, useForeground } from 'react-native-google-mobile-ads';
import settings, { googleAd } from './Settings';

import ScreenUtil from './ScreenUtil'; 
import global from './Global';

import  ViewMain from './ViewMain';

class App extends Component {
    
  constructor(props) {
      super(props);

      LogBox.ignoreLogs(['new NativeEventEmitter']); // Ignore log notification by message
      //LogBox.ignoreAllLogs(); //Ignore all log notifications

      this.inited = false;

      ScreenUtil.init();
      //global.init();

      this.bannerRef = React.createRef();

      const screen = Dimensions.get('screen');

      this.state = {
        inited: false,
        bottomBarHeight: -1000,
        flexHeight: 0,
        orientation: screen.width > screen.height ? 'landscape' : 'portrait',
        screenData: screen,
        
        appState: AppState.currentState,

        showAddGroup: {show: false, type: 'create group'},
        showEditGroup: {show: false, grp: null}, 
        showSettingProfile: false,
        showSettingPassword: false,
        showSettingGeneral: false,
        showChat: {show: false, grp: null},
        showTips: {show: false, title: '', tips: '', type: 'error'},
        showPwd: {show: false},
        showToast: '',
        showScanQRCode: {show: false},

        toastText: '',
        toastAnim: new Animated.Value(0),
        toastVisible: false,
      };
      this.inactiveTime = 0;

  }

  componentDidMount(){
    this.state.appState = AppState.currentState;
    //this.subscriptionAppState = AppState.addEventListener('change', this._handleAppStateChange);

    let self1 = this;
    /*
    setTimeout(function() {
      self1.unsubscribeNetState = NetInfo.addEventListener(state => {
          global.consoleLog("netState.type: "+ state.type);
          global.consoleLog("netState.isConnected1: "+ state.isConnected);
          self1.netStatus.isConnected = state.isConnected;
          self1.netStatus.type = state.type;
          //global.init();
      });
      //this.unsubscribeNetState();
    }, 5000);
    */

    this.listener1 = DeviceEventEmitter.addListener('cmd', (emitData) => {
      if (emitData.cmd == 'initLoaded') {
        if (global.isInited() == true) {
          global.clearCacheDirectory();
          this.subscriptionAppState = AppState.addEventListener('change', this._handleAppStateChange);
          this.setState({
            inited: true,
          })
        }
      }
      else if (emitData.cmd == 'showInputAppPwd') {
        //alert('need to input pwd')
        if (emitData.type === 'lock') {
          this.setState({showPwd:{show:true, type:'lock'}})
        }
        else {
          this.setState({showPwd:{show:true}})
        }
      }
      else if (emitData.cmd == 'showAddGroup') {
        this.setState({
          showAddGroup: {show: true, type:emitData.type},
        })
      }
      else if (emitData.cmd == 'showEditGroup') {
        this.setState({
          showEditGroup: {show:true, grp:emitData.grp}
        })
      }
      else if (emitData.cmd == 'showSettingProfile') {
        this.setState({
          showSettingProfile: true,
        })
      }
      else if (emitData.cmd == 'showSettingPassword') {
        this.setState({
          showSettingPassword: true,
        })
      }
      else if (emitData.cmd == 'showSettingGeneral') {
        this.setState({
          showSettingGeneral: true,
        })
      }
      else if (emitData.cmd == 'showChat') {
        this.setState({
          showChat: {show:true, grp:emitData.grp, netStatus:emitData.netStatus},
        })
      }
      else if (emitData.cmd == 'showTips') {
        /*
        this.setState({
          showTips: {show:true, title:emitData.title, tips:emitData.tips, type:emitData.type},
        })
          */
        //if (emitData.type === 'error') {
          Alert.alert(emitData.title, emitData.tips, [
            {
                text: ScreenUtil.getTextValue('close'),
                onPress: () => {},
            },
          ])
        //}
      }
      else if (emitData.cmd == 'showToast') {
        this.showToast(emitData.text);
      }
      else if (emitData.cmd == 'scanQRCode') {
        this.setState({
          showScanQRCode: {show:true,},
        })
      }
    })
  }

  componentWillUnmount() {
    //myBackgroundTimer.stopBackgroundTimer();
    if (this.subscriptionAppState) this.subscriptionAppState.remove();
    if (this.listener1) { this.listener1.remove(); }
  }

  _handleAppStateChange = (nextAppState) => {
    global.consoleLog('AppStatus '+this.state.appState+'/'+nextAppState);
    this.setState({appState: nextAppState});
    if (nextAppState === 'active') {
      let tNow = ( new Date()).getTime(); //ms
      if (this.inactiveTime + 3000 < tNow) {
        global.consoleLog('##authen / '+ global.enableBioAuthen)
        if (global.enableBioAuthen === true) {
          global.getCredentialsWithBiometry().then((res) => {
            global.consoleLog(res);
            if (res === null) {
              DeviceEventEmitter.emit('cmd', {cmd:'showInputAppPwd', type:'lock', });
            }
          })
        }
        else if (global.appPwd && global.appPwd.enable == true ) {
          DeviceEventEmitter.emit('cmd', {cmd:'showInputAppPwd', });
        }
      }
    }
    else if (nextAppState === 'inactive' || nextAppState === 'background') {
      this.inactiveTime = ( new Date()).getTime(); //ms
    }
  }

  render() {
    return (
      <SafeAreaInsetsContext.Consumer>
        {(insets) => {

          if (this.state.flexHeight == 0 || this.state.inited === false) {
            if (Platform.OS === 'android') {
              return (
                <View style={{ flex: 1, backgroundColor:'yellow' }} onLayout={e => {

                  if (this.state.orientation === 'landscape') {
                    alert("携帯電話を縦向きにしてください。\n\n画面の向きが横向きになっています。アプリは縦向きでのみ動作します。");
                    return;
                  }
                  
                  NativeModules.MyNativeModule.getDeviceDimensions().then((dimensions) => {
                    
                      console.log("Android getDeviceDimensions", dimensions);

                      const adHeight = googleAd.enable === true ? googleAd.bottomBarHeight : 0;

                      ScreenUtil.deviceWidth = dimensions.portrait.screenWidth;
                      ScreenUtil.deviceHeight = dimensions.portrait.screenHeight;
                      ScreenUtil.statusBarHeight = dimensions.portrait.statusBarHeight;
                      ScreenUtil.nativeBottomPadding = dimensions.portrait.navigationBarHeight;
                      ScreenUtil.flexHeight = ScreenUtil.deviceHeight-ScreenUtil.statusBarHeight-ScreenUtil.nativeBottomPadding - adHeight;
                      ScreenUtil.tabFlexHeight = ScreenUtil.flexHeight-ScreenUtil.scaleHeight(ScreenUtil.navigatorBarHeight);

                      this.setState({ flexHeight: ScreenUtil.flexHeight });
                      
                      if (googleAd.enable === true) {
                        mobileAds()
                          .initialize()
                          .then(adapterStatuses => {
                            // Initialization complete!
                            global.init();
                        });
                      }
                      else {
                        global.init();
                      }

                  }).catch((err) => {
                      alert("Android getDeviceDimensions error");
                  });

                  
                  const screen = Dimensions.get('screen');
                  console.log("📱 Dimensions:", screen);
                  console.log("📱 insets1:", insets);

                              var {x, y, width, height} = e.nativeEvent.layout;
                              console.log("SafeAreaView1 onLayout", {x, y, width, height});
      
                  
                  
                }} />
              );
            }

            return ( // iOS
              <View style={{ flex: 1 }} onLayout={e => {
                if (this.state.orientation === 'landscape') {
                  alert("携帯電話を縦向きにしてください。\n\n画面の向きが横向きになっています。アプリは縦向きでのみ動作します。");
                  return;
                }

                const adHeight = googleAd.enable === true ? googleAd.bottomBarHeight : 0;

                var {x, y, width, height} = e.nativeEvent.layout;
                console.log("init View onLayout", {x, y, width, height});
                ScreenUtil.flexHeight = height - insets.top - insets.bottom - adHeight;
                ScreenUtil.tabFlexHeight = ScreenUtil.flexHeight-ScreenUtil.scaleHeight(ScreenUtil.navigatorBarHeight);
                ScreenUtil.deviceWidth = width;
                ScreenUtil.deviceHeight = height;
                this.setState({ flexHeight: ScreenUtil.flexHeight });

                const statusBarHeight = insets.top;
                const navigationBarHeight = insets.bottom;
                const leftInset = insets.left;
                const rightInset = insets.right;


                console.log("init  View Safe Area Insets (" + this.state.orientation + "):", {
                  insets,
                  statusBar: statusBarHeight,
                  navigationBar: navigationBarHeight,
                  left: leftInset,
                  right: rightInset
                });

                if (this.state.orientation === 'portrait') {
                  ScreenUtil.statusBarHeight = statusBarHeight;
                  ScreenUtil.nativeBottomPadding = navigationBarHeight;
                }

                if (googleAd.enable === true) {
                        mobileAds()
                          .initialize()
                          .then(adapterStatuses => {
                            // Initialization complete!
                            global.init();
                        });
                      }
                      else {
                        global.init();
                      }
              }}/>
            );
          }

          if (Platform.OS === 'ios') {
            return (
              <SafeAreaView1 style={{ flex: 1, flexDirection:'column', }} onLayout={(e) => {
                
                if (this.inited === false) {
                  this.inited = true;

                  const {x, y, width, height} = e.nativeEvent.layout;
                  console.log("📐 SafeAreaView Layout (" + this.state.orientation + "):", {x, y, width, height});

                  const statusBarHeight = insets.top;
                  const navigationBarHeight = insets.bottom;
                  const leftInset = insets.left;
                  const rightInset = insets.right;

                  console.log("📏 SafeAreaView1 Safe Area Insets (" + this.state.orientation + "):", {
                    insets,
                    statusBar: statusBarHeight,
                    navigationBar: navigationBarHeight,
                    left: leftInset,
                    right: rightInset
                  });

                  if (this.state.orientation === 'portrait') {
                    ScreenUtil.statusBarHeight = statusBarHeight;
                    ScreenUtil.nativeBottomPadding = navigationBarHeight;
                  }
                }
            }}>
                <View style={{ 
                      height: ScreenUtil.deviceHeight-ScreenUtil.statusBarHeight-ScreenUtil.nativeBottomPadding,
                      width: ScreenUtil.deviceWidth, }}>
                  {this.renderMainPage()}
                </View>
              </SafeAreaView1>)
          }
    
          return ( // Android
            <SafeAreaView1 style={{flex:1,}} onLayout={(e) => {
                if (this.inited === false) {
                  this.inited = true;

                }
            }}>
              {this.renderMainPage()}
            </SafeAreaView1>
          );
  
    }}
      </SafeAreaInsetsContext.Consumer>
    )
  }

  render1() {
    if (this.state.flexHeight == 0 || this.state.inited === false) {
      return (
          <View style={{ flex: 1, width:ScreenUtil.deviceWidth, height:ScreenUtil.deviceHeight, backgroundColor:'red'}} onLayout={e => {
              console.log('@#@#@# '+ e.nativeEvent.layout.height + '@#@#@# '+ ScreenUtil.nativeDeviceHeight );
              ScreenUtil.flexHeight = e.nativeEvent.layout.height - ScreenUtil.nativeSatusBarHeight - ScreenUtil.nativeBottomPadding;
              ScreenUtil.tabFlexHeight = ScreenUtil.flexHeight - ScreenUtil.scaleHeight(ScreenUtil.navigatorBarHeight);
              this.setState({flexHeight: e.nativeEvent.layout.height})
          }}>
          </View>
      );
    }

    if (Platform.OS=='ios' ) {
      if (ScreenUtil.isOnIPad==true) {
        return (
          <View style={{flex:1}}>
            {this.renderMainPage()}
          </View>
        )
      }
      return (
        <View style={{flex:1}}>
          <View style={{backgroundColor:'transparent',
              height:ScreenUtil.nativeSatusBarHeight}} >
                {/*
            <StatusBar 
              barStyle={'light-content'} />
                */}
          </View>

          {this.renderMainPage()}
          
        </View>
      )
    }
    return (
      <View style={{flex:1}}>
        <StatusBar
          barStyle={'dark-content'}
          //backgroundColor={backgroundStyle.backgroundColor}
        />
        {this.renderMainPage()}
      </View>
    )
  }

  showToast = (text) => {
    //console.log('showToast')
    this.state.toastText = text;
    this.state.toastVisible = true;
    this.setState({
      toastText:text,
      toastVisible: true,
    })
    Animated.timing(this.state.toastAnim, {
      toValue: 1,
      duration: 300,
      useNativeDriver: true,
    }).start(() => {
      setTimeout(() => {
        Animated.timing(this.state.toastAnim, {
          toValue: 0,
          duration: 300,
          useNativeDriver: true,
        }).start(() => {this.setState({toastVisible:false})});
      }, 2000);
    });
  };

  renderMainPage1() {
    return (
      <View style={{flex:1, flexDirection:'column', alignItems:'center', justifyContent:'center', backgroundColor:'red',}} />
    )
  }
  renderMainPage() {
    return (
      <View style={{height:'100%', width:'100%', backgroundColor:'red', flexDirection:'column'
            }}>
        <View style={{height:ScreenUtil.flexHeight, width:ScreenUtil.deviceWidth, backgroundColor:'transparent', }}>
          <ViewMain />
        </View>
        {googleAd.enable === true && 
          <BannerAd ref={this.bannerRef} unitId={TestIds.ADAPTIVE_BANNER} size={BannerAdSize.LARGE_ANCHORED_ADAPTIVE_BANNER} />
        }
      </View>
    )
  }
}

const styles = StyleSheet.create({
  sectionContainer: {
    marginTop: 32,
    paddingHorizontal: 24,
  },
  sectionTitle: {
    fontSize: 24,
    fontWeight: '600',
  },
  sectionDescription: {
    marginTop: 8,
    fontSize: 18,
    fontWeight: '400',
  },
  highlight: {
    fontWeight: '700',
  },
});

export default App;
