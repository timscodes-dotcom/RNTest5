import {
    Dimensions,
    PixelRatio,
    Platform,
    StatusBar,
    NativeModules,
} from 'react-native';

import * as RNLocalize from "react-native-localize";

const windowWidth = Dimensions.get('window').width;
const windowHeight = Dimensions.get('window').height;
const windowAspectRatio = windowHeight/windowWidth;

const deviceWidth = windowWidth;      //设备的宽度
//const deviceHeight = Dimensions.get('window').height;    //设备的高度
const deviceHeight = Platform.OS === 'android' && Platform.Version > 26 ? Dimensions.get('screen').height - StatusBar.currentHeight : windowHeight;
let fontScale = PixelRatio.getFontScale();                      //返回字体大小缩放比例
 
let pixelRatio = PixelRatio.get();      //当前设备的像素密度
const defaultPixel = 2;                           //iphone6的像素密度
//px转换成dp
const w2 = 375.0 / defaultPixel;
const h2 = 812.0 / defaultPixel;
const scale = Math.min(deviceHeight / h2, deviceWidth / w2);   //获取缩放比例
const _scaleWidth = deviceWidth / w2;
const _scaleHeight = deviceHeight / h2;
/**
 * 设置text为sp
 * @param size sp
 * return number dp
 */

// iPhoneX
const iPhoneX_WIDTH = 375;
const iPhoneX_HEIGHT = 812;
const iPhoneXR_WIDTH = 414;
const iPhoneXR_HEIGHT = 896;
const iPhoneX = Platform.OS === 'ios' &&
                ((windowHeight === iPhoneX_HEIGHT && windowWidth === iPhoneX_WIDTH) ||
                    (windowHeight === iPhoneX_WIDTH && windowWidth === iPhoneX_HEIGHT)) ? true : false;
const iPhoneXR = Platform.OS === 'ios' &&
                    ((windowHeight === iPhoneXR_HEIGHT && windowWidth === iPhoneXR_WIDTH) ||
                        (windowHeight === iPhoneXR_WIDTH && windowWidth === iPhoneXR_HEIGHT)) ? true : false;
                       
export const const_language_en = 0;
export const const_language_cht = 1;
export const const_language_chs = 2;
export const const_languages = ['en', 'cht', 'chs'];

var ScreenUtil = {

    deviceWidth: deviceWidth,
    deviceHeight: iPhoneX || iPhoneXR ? deviceHeight-34 : deviceHeight,
    
    flexHeight: 0,
    tabFlexHeight: 0, 

    iPhoneX: iPhoneX || iPhoneXR,
    isOnIPad: false,
    nativeSatusBarHeight: Platform.OS === 'ios' ? NativeModules.MyNativeModule.getStatusBarHeight() : 0,
    nativeBottomPadding: Platform.OS === 'ios' ? NativeModules.MyNativeModule.getBottomPadding() : 0,

    navigatorBarHeight: 55,

    textScale: scale,
    fontScale: fontScale,

    currentTab: 'Tab1',
    currentTab_remember : '',

    appState: 'active',
    appInactiveTime: 0,

    adjustHeight: {},
    adjustLineHeight: {},
    adjustWidth: {},
    adjustTextSize: {},

    lightMode: 'day',
    lang: 'en',//'cht',

    textSize: 'big',

    textSizeList: {
        tabTitle: {
            small: {fontWeight:'bold', fontSize:14},
            medium: {fontWeight:'bold', fontSize:17},
            big: {fontWeight:'bold', fontSize:20},
        },
        tabLable: {
            small: {fontSize:10},
            medium: {fontSize:13},
            big: {fontSize: 15},
        },
        settingLable: {
            small: {fontSize:10},
            medium: {fontSize:13},
            big: {fontSize: 15},
        },
        settingInput: {
            small: {fontSize:9},
            medium: {fontSize:11},
            big: {fontSize: 13},
        },
        msgTips:{
            small: {fontSize:11},
            medium: {fontSize:13},
            big: {fontSize: 15},
        },
        tabSettingsName: {
            small: {fontWeight:'bold', fontSize:19},
            medium: {fontWeight:'bold', fontSize:22},
            big: {fontWeight:'bold', fontSize:25},
        },
    },

    getTextSize(key) {
        let tmp =   JSON.parse(JSON.stringify( this.textSizeList[key]['big'] ));
        tmp.fontSize = this.scaleText(tmp.fontSize);
        return tmp;
    },

    textColor: {
        'backGround':{'day':'#ffffff','night':'#ffffff'},
        'backGroundGrey':{'day':'#f2f2f2','night':'#f2f2f2'},
        'backGrounda':{'day':'rgba(52,52,52,0.5)','night':'rgba(21,31,44,1)'},
        'keyColor':{'day':'#40a7e3','night':'#40a7e3'},
        'highlight1':{'day':'#151f2c','night':'#ffffff'},
        'txtGrey':{'day':'#6a7a8e','night':'#6a7a8e'},
        'placeholderTextColor':{'day':'#d2d2d2','night':'#d2d2d2'},
    },

    textValue: {
        'quote':      {'en':'レート',            'cht':'行情'},
        'tabQuickTrade':           {'en':'レート',            'cht':'價格'},
        'loading':           {'en':'loading',            'cht':'loading'},
        'tabHome':  {'en':'Chat Group', 'cht':'聊天組', 'chs':'聊天组'},
        'tabSetting':  {'en':'Setting', 'cht':'設置', 'chs':'设置'},
        'addGroup': {'en':'Add Group', 'cht':'新增聊天組', 'chs':'新增聊天组'},
        'groupName': {'en':'Group Name', 'cht':'聊天組名字', 'chs':'聊天组名字'},
        'ipAddress': {'en':'IP Address', 'cht':'IP地址', 'chs':'IP地址'},
        'portText': {'en':'Text Port', 'cht':'文字端口', 'chs':'文字端口'},
        'portFile': {'en':'File Port', 'cht':'文件端口', 'chs':'文件端口'},
        'autoConnect': {'en':'Auto Connect', 'cht':'自動連接', 'chs':'自动连接'},
        'createGroup': {'en':'Create Group', 'cht':'創建聊天組', 'chs':'创建聊天组'},
        'joinGroup': {'en':'Join Group', 'cht':'加入聊天組', 'chs':'加入聊天组'},
        'selIpAddress': {'en':'Select IP Address', 'cht':'選擇IP地址', 'chs':'选择IP地址'},
        'publicIpAddress': {'en':'Public IP Address', 'cht':'公開的IP地址', 'chs':'公开的IP地址'},
        'publicPortText': {'en':'Public Text Port', 'cht':'公開的文字端口', 'chs':'公开的文字端口'},
        'publicPortFile': {'en':'Public File Port', 'cht':'公開的文件端口', 'chs':'公开的文件端口'},
        'editGroup': {'en':'Edit Group', 'cht':'編輯聊天組', 'chs':'编辑聊天组'},
        'delGroup': {'en':'Delete Group', 'cht':'刪除聊天組', 'chs':'删除聊天组'},
        'save': {'en':'Save', 'cht':'保存', 'chs':'保存'},
        'qrCode': {'en':'QR Code', 'cht':'QR Code', 'chs':'QR Code'},
        'genQrCode': {'en':'Generate QR Code', 'cht':'生成QRCode', 'chs':'生成QRCode'},
        'qrCodeExpireTime': {'en':'QR Code\'s Expire Time', 'cht':'QR碼的有效時間', 'chs':'QR码的有效时间'},
        'qrCodeErr1': {'en':'Fail to parse the QR Code!', 'cht':'解析QR碼失敗', 'chs':'解析QR码失败'},
        'err': {'en':'Error', 'cht':'錯誤', 'chs':'错误'},
        'errDisconnect': {'en':'Server is disconnected!', 'cht':'服務器已斷開！', 'chs':'服务器已断开！'},
        'profile': {'en':'Profile', 'cht':'個人資料', 'chs':'个人资料'},
        'role': {'en':'Role', 'cht':'Role', 'chs':'Role'},
        'changeIcon': {'en':'Change Icon', 'cht':'修改頭像', 'chs':'修改头像'},
        'changeColor': {'en':'Change Color', 'cht':'修改顏色', 'chs':'修改颜色'},
        'nickName': {'en':'Nick Name', 'cht':'暱稱', 'chs':'昵称'},
        'members': {'en':'Members', 'cht':'成員', 'chs':'成员'},
        'defaultProfile': {'en':'Default Profile', 'cht':'默認個人資料', 'chs':'默认个人资料'},
        'generalSettings': {'en':'General Settings', 'cht':'一般設定', 'chs':'一般设定'},
        'pwd': {'en':'Password', 'cht':'密碼', 'chs':'密码'},
        'pwdConfirm': {'en':'Confirm Password', 'cht':'確認密碼', 'chs':'确认密码'},
        'pwdConfig': {'en':'Password Setting', 'cht':'密碼設置', 'chs':'密码设置'},
        'pwdEnable': {'en':'Enable Password', 'cht':'啟用密碼', 'chs':'启用密码'},
        'fingerPrint': {'en':'FingerPrint', 'cht':'指紋', 'chs':'指纹'},
        'icon': {'en':'Icon', 'cht':'頭像', 'chs':'头像'},
        'lang': {'en':'Language', 'cht':'語言', 'chs':'语言'},
        'langEn': {'en':'English', 'cht':'英文', 'chs':'英文'},
        'langCht': {'en':'Traditional Chinese', 'cht':'繁體中文', 'chs':'繁体中文'},
        'langChs': {'en':'Simplify Chinese', 'cht':'簡體中文', 'chs':'简体中文'},
        'textSize': {'en':'Text Size', 'cht':'字體大小', 'chs':'字体大小'},
        'textSmall': {'en':'Small', 'cht':'小', 'chs':'小'},
        'textMedium': {'en':'Medium', 'cht':'中', 'chs':'中'},
        'textBig': {'en':'Big', 'cht':'大', 'chs':'大'},
        'pickColor': {'en':'Pick Color', 'cht':'挑選顏色', 'chs':'挑选颜色'},
        'picked': {'en':'Picked', 'cht':'已挑選的', 'chs':'已挑选的'},
        'confirm': {'en':'Confirm', 'cht':'確定', 'chs':'确定'},
        'pwdErr1': {'en':'Please input password!', 'cht':'請輸入密碼！', 'chs':'请输入密码！'},
        'pwdErr2': {'en':'Fail to confirm password!', 'cht':'輸入密碼不一致！', 'chs':'输入密码不一致！'},
        'pwdErr3': {'en':'Wrong password!', 'cht':'密碼錯誤！', 'chs':'密码错误！'},
        'lockApp': {'en':'Lock App Screen', 'cht':'鎖定App屏幕', 'chs':'锁定App屏幕'},
        'help': {'en':'Help', 'cht':'幫助', 'chs':'帮助'},
        'homepage': {'en':'Homepage', 'cht':'主頁', 'chs':'主页'},
        'delGrpConfirm': {'en':'Are you sure to delete this chat group?', 'cht':'確定刪除聊天組', 'chs':'确定删除聊天组'},
        'yes': {'en':'Yes', 'cht':'是', 'chs':'是'},
        'no': {'en':'No', 'cht':'否', 'chs':'否'},
        'close': {'en':'Close', 'cht':'關閉', 'chs':'关闭'},
        'copy1': {'en':'Copy to Clipboard', 'cht':'複製到剪切板', 'chs':'复制到剪切板'},
        'copySuc1': {'en':'Copied', 'cht':'複製成功', 'chs':'复制成功'},
        'wanIp1': {'en':'WAN IP Address', 'cht':'外網IP地址', 'chs':'外网IP地址'},
        'wanIpErr1': {'en':'Fail to obtain WAN IP Address', 'cht':'獲取外網IP地址失敗', 'chs':'获取外网IP地址失败'},
        'use1': {'en':'Use', 'cht':'使用', 'chs':'使用'},
    },

    init() {
    },

    getTextValue(name)
    {
        if (this.textValue[name]){
             return this.textValue[name][this.lang];
        }
        else {
            console.log('#### no ' + name);
        }
        return '';
    },

    getTextColor(name)
    {
        if (this.textColor[name]){
            return this.textColor[name][this.lightMode];
       }
       else {
            console.log('#### no ' + name);
       }
        return this.textColor['backGround'][this.lightMode];
    },

    scaleText(size) {
        if (this.adjustTextSize[size] == null) {
            let size1 = Math.round((size * scale)  / fontScale * 1.1 + 0.5);
            this.adjustTextSize[size] = size1 / defaultPixel;
        }
        return this.adjustTextSize[size];
    },

    scaleLineHeight(size, txtScale) {
        if (this.adjustLineHeight[size] == null) {
            txtScale = txtScale || 1.2;
            txtScale = txtScale * 1.1;
            let size1 = Math.round((size * scale * txtScale)  / fontScale  + 0.5);
            this.adjustLineHeight[size] = size1 / defaultPixel;
        }
        return this.adjustLineHeight[size];
    },

    lineHeight(fontSize) {
        let size = this.scaleText(fontSize);
        const multiplier = (size > 20) ? 1.5 : 1;
        return Math.floor(size + (size * multiplier) + 0.5);
    },

    /**
     * 屏幕适配,缩放size , 默认根据宽度适配，纵向也可以使用此方法
     * 横向的尺寸直接使用此方法
     * 如：width ,paddingHorizontal ,paddingLeft ,paddingRight ,marginHorizontal ,marginLeft ,marginRight
     * @param size 设计图的尺寸
     * @returns {number}
     */
     scaleWidth(size) {
        if (this.adjustWidth[size] == null) {
            this.adjustWidth[size] = Math.round( size * _scaleWidth *1.0 / defaultPixel + 0.5);
        }
        return this.adjustWidth[size];
    },

    /**
     * 屏幕适配 , 纵向的尺寸使用此方法应该会更趋近于设计稿
     * 如：height ,paddingVertical ,paddingTop ,paddingBottom ,marginVertical ,marginTop ,marginBottom
     * @param size 设计图的尺寸
     * @returns {number}
     */
    scaleHeight(size) {
        if (this.adjustHeight[size] == null) {
            this.adjustHeight[size] = Math.round( size * _scaleHeight * 1.0 / defaultPixel + 0.5);
        }
        return this.adjustHeight[size];      
    },

    init() {
        const language = RNLocalize.getLocales()[0].languageCode; //en, ja, zh
        const languageTag = RNLocalize.getLocales()[0].languageCode;
        console.log(`## your language is : ${language}/${languageTag}`);
        
        if (language === 'zh') {
            if (languageTag.lastIndexOf('cn') >= 0 || languageTag.lastIndexOf('CN') >= 0) {
                this.lang = 'chs';
            }
            else {
                this.lang = 'cht';
            }
        }
        else if (language === 'ja') {
            this.lang = 'jp';
        }
        else {
            this.lang = 'en';
        }

        console.log('##height: ' + this.nativeSatusBarHeight)
        if (Platform.OS == 'android') {
            //console.log('@@@@@@debug here ' + this.nativeSatusBarHeight);
            //console.log('isPad: '+DeviceInfo.getDeviceType());
            //console.log(Dimensions.get('window').height + ' / ' + Dimensions.get('screen').height);
        }
        else {
            NativeModules.MyNativeModule.isOnIPad((ret) => {
                console.log('$$$$$$$$ is on ipad ' + ret);
                if (ret == true) {
                    this.isOnIPad = true;
                } 
              })
            /*
            NativeModules.MyNativeModule.getStatusBarHeight()
                .then(
                    (h) => {
                        this.nativeSatusBarHeight = h;
                        //console.log('nativeSatusBarHeight: '+ this.nativeSatusBarHeight + ' / ' + this.statusBarHeight );
                    }
                )
                /*
                .catch(
                    (code, err) => {
                    alert(err)
                    }
                )
                */
            /*
            NativeModules.MyNativeModule.getBottomPadding()
            .then(
                (values) => {
                    this.nativeBottomPadding = values.bottom;
                    this.nativeDeviceHeight = values.height;
                    //console.log('nativeBottomPadding: '+ this.nativeBottomPadding  );
                    //console.log('deviceH: '+ this.deviceHeight + ' / ' + values.height + ' / ' + Dimensions.get('screen').height + ' / ' + Dimensions.get('window').height );
                }
            )
            
            .catch(
                (code, err) => {
                alert(err)
                }
            )
            */
        }

    },
};

export default ScreenUtil;
