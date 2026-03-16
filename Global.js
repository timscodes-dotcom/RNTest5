import{DeviceEventEmitter,NativeModules, NativeEventEmitter,Platform,} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';

import ScreenUtil, {const_languages} from './ScreenUtil'; 

import RNFS from 'react-native-fs';

var global = {

    inited: {nativeModules:false, data:false},
    filesDir: '',

    g_StorageBuf : {},

    enableBioAuthen: false,

    chatGrpList: [],
    showChatGrp: '',

    inputHeight: ScreenUtil.scaleHeight(45),
    titleHeight: ScreenUtil.scaleHeight(50),

    consoleLog(log) {
        console.log(log);
    },

    isInited() {
        return (this.inited.nativeModules==true && this.inited.data==true);
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AsyncStorageMultiSet(keyValuePairs) {
        AsyncStorage.multiSet(keyValuePairs, function(errs){
            if(errs){
                //TODO：存储出错
                alert('saveClientBufferMultiSet fail!!');
                return;
            }
            //alert('数据保存成功!');
        });
    },

    AsyncStorageSetItem(key, value) {
        try {
            AsyncStorage.setItem(key, value);
        } catch (error) {
            // Error saving data
            alert('fail to save '+key);
        }
    },

    AsyncStorageRemoveItem(key) {
        try {
            AsyncStorage.removeItem(key);
        } catch (error) {
            // Error remove data
            alert('fail to remove '+key);
        }
    },

    AsyncStorageGetAllKeys(func) {
        AsyncStorage.getAllKeys((err, keys) => {
            if(err != null){
                alert("getAllKeys error:"+err.message);
                return;
            }

            AsyncStorage.multiGet(keys, (err, stores) => {
                if(err != null){
                    alert("multiGet error:"+err.message);
                    return;
                }

                func(stores);
            });
        });
       /*
       let keys = ['chartSettings','installComplete','autoLogin','lastLoginUser','lastLoginPwd','lang','saveLoginUser','saveLoginPwd']
       AsyncStorage.multiGet(keys, (err, stores) => {
            if(err != null){
                alert("multiGet error:"+err.message);
                return;
            }

            func(stores);
        });
        */
    },

    loadAsyncStorage(stores) {
        stores.map((result, i, store) => {
            // get at each store's key/value so you can work with it
            let key = store[i][0];
            //let value = store[i][1];
            this.g_StorageBuf[key] = store[i][1];
        });

        this.initData();
        this.inited.data = true;

        DeviceEventEmitter.emit('cmd', {cmd:'initLoaded',});
    },

    saveClientBufferMultiSet(keyValuePairs) {
        for (let i = 0; i < keyValuePairs.length; i++) {
            this.g_StorageBuf[keyValuePairs[i][0]] = keyValuePairs[i][1];      
        }        
        /*
        AsyncStorage.multiSet(keyValuePairs, function(errs){
            if(errs){
                //TODO：存储出错
                alert('saveClientBufferMultiSet fail!!');
                return;
            }
            //alert('数据保存成功!');
        });
        */
       this.AsyncStorageMultiSet(keyValuePairs);
    },

    saveClientBuffer(key, value) {
        this.g_StorageBuf[key] = value;
        /*
        try {
            AsyncStorage.setItem(key, value);
        } catch (error) {
            // Error saving data
            alert('fail to save '+key);
        }
        */
        this.AsyncStorageSetItem(key, value);
    },

    removeClientBuffer(key) {
        this.g_StorageBuf[key] = null;
        this.AsyncStorageRemoveItem(key);
    },

    getClientBufferStr(key) {
        return this.g_StorageBuf[key];
    },

    async getWANIp(_force) {
        let self = this;
        if (_force !== true) {
            let tNow = (new Date()).getTime();
            if (self.WANIp.refreshTimeStamp + 60*60*1000 >= tNow) return;
        }
        let bSuc = false;
        try {
            let controller = new AbortController();
            setTimeout(() => {if (controller) controller.abort()}, 5000);
            let response = await fetch('http://ip-api.com/json', {
                method: 'GET',
                cache: "no-cache",
                signal: controller.signal,
                //headers: {},
                //body: JSON.stringify(postData),
            })
            let json = await response.json();
            console.log(JSON.stringify(json))
            if (json["status"]==='success' && json["query"] && json["countryCode"]) {
                bSuc = true;
                self.WANIp.ip = json["query"];
                self.WANIp.countryCode = json["countryCode"];
                self.WANIp.refreshTimeStamp = (new Date()).getTime();
                self.saveClientBuffer('WANIp', JSON.stringify(self.WANIp));
            }
        }catch(error) {
            global.consoleLog(error);
        }
        if (bSuc === false) {
            try {
                let controller = new AbortController();
                setTimeout(() => {if (controller) controller.abort()}, 5000);
                let response = await fetch('https://ipinfo.io/json', {
                    method: 'GET',
                    cache: "no-cache",
                    signal: controller.signal,
                    //headers: {},
                    //body: JSON.stringify(postData),
                })

                let json = await response.json();
                console.log(JSON.stringify(json))
                if (json["ip"] && json["country"]) {
                    bSuc = true;
                    self.WANIp.ip = json["ip"];
                    self.WANIp.countryCode = json["countryCode"];
                    self.WANIp.refreshTimeStamp = (new Date()).getTime();
                    self.saveClientBuffer('WANIp', JSON.stringify(self.WANIp));
                }
            }catch(error) {
                global.consoleLog(error);
            }
        }
    },

    initData() {
        let _tmp = this.getClientBufferStr('chatGroupList') || "[]";
        this.chatGrpList = JSON.parse(_tmp);
        for (let i = 0; i < this.chatGrpList.length; i++) {
            this.chatGrpList[i].chatGrpStatus = 'disconnect';
        }
        this.consoleLog('@#@#@# '+_tmp);

        _tmp = this.getClientBufferStr('yourProfile') || JSON.stringify({name:'Anonymous', icon:{font:'MaterialIcons', name:'tag-faces', color:ScreenUtil.getTextColor('keyColor')}});
        this.yourProfile = JSON.parse(_tmp);

        _tmp = this.getClientBufferStr('generalSettings') || JSON.stringify({lang:0, textSize:2});
        this.generalSettings = JSON.parse(_tmp);
        ScreenUtil.lang = const_languages[this.generalSettings.lang];

        _tmp = this.getClientBufferStr('appPwd') || JSON.stringify({enable:false, pwd:''});
        this.appPwd = JSON.parse(_tmp);

        _tmp = this.getClientBufferStr('enableBioAuthen') || "0";
        this.enableBioAuthen = _tmp === "1"?true:false;

        _tmp = this.getClientBufferStr('WANIp') || JSON.stringify({ip:'', countryCode:'', refreshTimeStamp:0}); //JP
        this.WANIp = JSON.parse(_tmp);
        let self = this;
        self.getWANIp();
        setInterval(() => {
            self.getWANIp();
        }, 30000);

        this.deviceID = this.getClientBufferStr('deviceID') || '';
        if (this.deviceID === '' || this.deviceID.length!=8) {
            this.deviceID = this.randomString(8);
            this.saveClientBuffer('deviceID', this.deviceID);
        }
        NativeModules.MyNativeModule.jsonCmd(JSON.stringify({
                    cmd:"setDeviceID", 
                    deviceID:this.deviceID, 
                })).then(
            (values) => {
            }
        );
        this.consoleLog('@@@@ device id: '+this.deviceID)
    },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
    init() {
        let self = this;

        console.log('Global.init NativeModules.MyNativeModule.init');
        NativeModules.MyNativeModule.init().then(
            (values) => {
                this.consoleLog('MyNativeModule.init : ' + JSON.stringify(values) );
                if (Platform.OS == 'ios') {
                    /*
                    this.consoleLog('homeDir: '+ values.homeDir);
                    this.consoleLog('cachePath: '+ values.cachePath);
                    self.g_fileFolder = values.cachePath;
                    this.consoleLog('tstWriteFile: '+ values.tst);
                    */
                    self.filesDir = values.homeDir;
                    self.cacheDir = values.cachePath;
                }
                else {
                    /*
                    this.consoleLog('MyNativeModule.init : ' + values.filesDir+' / ' + values.packageName );
                    self.g_fileFolder = values.filesDir;
                    */
                   self.filesDir = values.filesDir;
                   self.cacheDir = values.cacheDir;
                }
                self.inited.nativeModules = true;
                DeviceEventEmitter.emit('cmd', {cmd:'initLoaded',});
            }
        )
        console.log('Global.init DeviceEventEmitter listeners');

        if (Platform.OS == 'ios') {
            //const {MyNativeModule} = NativeModules;
            this.iosEmitter = new NativeEventEmitter(NativeModules.MyNativeModule);
            this.iosSubscription = this.iosEmitter.addListener(
                'iosEvent', (msgBody) => { 
                    this.consoleLog("iosEmitter: "+JSON.stringify(msgBody))
                    if (msgBody.cmd == 'native' && msgBody.msg !== "testIOSEvent1" && msgBody.msg !== "testCallbackFunc" && msgBody.msg !== "stringFromJNI: onTestMessage") {
                        //global.consoleLog("ios: "+msgBody.msg);
                        //let data = JSON.parse(msgBody.msg);
                        self.handleNativeMsg(msgBody.msg);
                    }
                }
            )
        }
		else if (Platform.OS == 'android') {
            this.listener1 = DeviceEventEmitter.addListener('cmd', (emitData) => {
                this.consoleLog('android DeviceEventEmitter: '+JSON.stringify(emitData))
				if (emitData.cmd == 'native' && emitData.msg !== "testIOSEvent1" && emitData.msg !== "testCallbackFunc" && emitData.msg !== "stringFromJNI: onTestMessage") {
					this.handleNativeMsg(emitData.msg);
				}
            });
        }

        this.AsyncStorageGetAllKeys(this.loadAsyncStorage.bind(this));
    },
	
	release() {
        if (Platform.OS == 'android' && this.listener1) { this.listener1.remove(); }
        if (Platform.OS == 'ios' && this.iosSubscription) { this.iosSubscription.remove(); }
    },

    testCallbackFunc() {
        if (Platform.OS == 'ios') {
            NativeModules.MyNativeModule.getAppVersion((res)=>{
              console.log('ios module callback: '+res);
            })
          }
          else {
            NativeModules.MyNativeModule.testCallbackFunc('testCallbackFunc').then((res)=>{
                console.log('testCallbackFunc promise.resolve: '+res)
            });
          }
    },

    handleNativeMsg(emitData) {
        //console.log('handleNativeMsg: '+JSON.stringify(emitData));
        //console.log(typeof(emitData));
        const obj = JSON.parse(emitData);

        if (obj.action === 'uploadFile' && obj.grpKey && obj.msgId) {
            DeviceEventEmitter.emit(obj.grpKey+'_'+obj.msgId, obj);
            return;
        }
        else if (obj.action === 'downloadFile' && obj.grpKey && obj.msgId) {
            DeviceEventEmitter.emit(obj.grpKey+'_'+obj.msgId, obj);
            return;
        }
        
        DeviceEventEmitter.emit(obj.grpKey, obj);
    },

    randomString(length) {
        var result           = '';
        var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
        var charactersLength = characters.length;
        for ( var i = 0; i < length; i++ ) {
           result += characters.charAt(Math.floor(Math.random() * charactersLength));
        }
        return result;
    },

    async deleteCacheFile(file) {
        const cacheFilePath = `${RNFS.CachesDirectoryPath}/${file}`;

        try {
            // 检查文件是否存在
            const fileExists = await RNFS.exists(cacheFilePath);
            if (fileExists) {
                // 删除文件
                await RNFS.unlink(cacheFilePath);
                console.log('文件已删除')
            } else {
                console.log('文件不存在')
            }
        } catch (error) {
            console.error('删除文件时出错:', error);
        }
    },

    async clearCacheDirectory() {
        try {
          const files = await RNFS.readDir(RNFS.CachesDirectoryPath);
          for (const file of files) {
            console.log(file.path);
            await RNFS.unlink(file.path);
          }
          console.log('缓存目录已清空');
        } catch (error) {
          console.log('清空缓存时出错:', error);
          //Alert.alert('清空失败', error.message);
        }
    },

};

export default global;