import React, { Component } from 'react';
import { View, TouchableOpacity, Text, NativeModules, Platform, FlatList, RefreshControl, ScrollView, TextInput, Switch, Image,
    DeviceEventEmitter, Keyboard, Alert, findNodeHandle, Modal } from 'react-native';
import PropTypes from 'prop-types';
import { SelectList } from 'react-native-dropdown-select-list';
import Clipboard from '@react-native-clipboard/clipboard';
import QRCode from 'react-native-qrcode-svg';
import RNFS from 'react-native-fs';

import mobileAds, { BannerAd, BannerAdSize, TestIds, useForeground } from 'react-native-google-mobile-ads';
import settings, { googleAd } from './Settings';

import ScreenUtil from './ScreenUtil'; 
import global from './Global';

export default class ViewMain extends Component {
    constructor(props) {
        super(props);
        this.state = {
            ipOptions: [],
            selectedIp: '',
            port: '8899',
            showQrCode: false,
            qrTitle: '二维码',
            qrValue: '',
            webServerRunning: false,
            uploadDir: '',
            uploadFiles: [],
        };
    }

    async componentDidMount() {
        try {
            const ipsStr = await NativeModules.MyNativeModule.jsonCmd(JSON.stringify({cmd:"GetLocalIPAddress"}));
            console.log('GetLocalIPAddress:'+ipsStr);

            const ipsJson = JSON.parse(ipsStr || '{}');
            const allAddress = Array.isArray(ipsJson.allAddress) ? ipsJson.allAddress : [];
            const ipOptions = allAddress
                .filter(item => typeof item === 'string' && item.length > 0)
                .map(item => ({ key: item, value: item }));

            this.setState({
                ipOptions,
                selectedIp: ipOptions.length > 0 ? ipOptions[0].value : '',
            });
        }
        catch (e) {
            console.log('GetLocalIPAddress error:', e);
            Alert.alert('提示', '获取本机IP失败');
        }
    }

    async componentWillUnmount() {
        if (this.state.webServerRunning) {
            try {
                await NativeModules.MyNativeModule.jsonCmd(JSON.stringify({cmd:'stopWebServer'}));
            }
            catch (e) {
            }
        }
    }

    startWebServer = async () => {
        const { selectedIp, port } = this.state;
        if (!selectedIp) {
            Alert.alert('提示', '请先选择IP');
            return;
        }
        const portNum = parseInt(port, 10);
        if (!portNum || portNum < 1 || portNum > 65535) {
            Alert.alert('提示', '端口范围应为 1-65535');
            return;
        }

        try {
            const retStr = await NativeModules.MyNativeModule.jsonCmd(JSON.stringify({
                cmd: 'startWebServer',
                ip: selectedIp,
                port: portNum,
            }));
            const ret = JSON.parse(retStr || '{}');
            if (ret.ret === 'ok') {
                this.setState({
                    webServerRunning: true,
                    uploadDir: ret.uploadDir || this.state.uploadDir,
                }, () => {
                    this.refreshUploadFiles();
                });
                Alert.alert('启动成功', ret.url || `http://${selectedIp}:${portNum}`);
            }
            else {
                Alert.alert('启动失败', ret.ret || retStr || '未知错误');
            }
        }
        catch (e) {
            Alert.alert('启动失败', String(e));
        }
    }

    stopWebServer = async () => {
        try {
            const retStr = await NativeModules.MyNativeModule.jsonCmd(JSON.stringify({cmd:'stopWebServer'}));
            const ret = JSON.parse(retStr || '{}');
            if (ret.ret === 'ok') {
                this.setState({ webServerRunning: false });
                Alert.alert('提示', '服务器已停止');
            }
            else {
                Alert.alert('停止失败', ret.ret || retStr || '未知错误');
            }
        }
        catch (e) {
            Alert.alert('停止失败', String(e));
        }
    }

    copyServerAddress = (serverAddress) => {
        if (!serverAddress) {
            Alert.alert('提示', '请先选择IP并填写端口');
            return;
        }
        Clipboard.setString(serverAddress);
        Alert.alert('已复制', serverAddress);
    }

    openQrCode = (serverAddress) => {
        if (!serverAddress) {
            Alert.alert('提示', '请先选择IP并填写端口');
            return;
        }
        this.setState({
            showQrCode: true,
            qrTitle: '服务器地址二维码',
            qrValue: serverAddress,
        });
    }

    formatFileSize = (size) => {
        const n = Number(size || 0);
        if (n < 1024) return `${n} B`;
        if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KB`;
        if (n < 1024 * 1024 * 1024) return `${(n / 1024 / 1024).toFixed(1)} MB`;
        return `${(n / 1024 / 1024 / 1024).toFixed(1)} GB`;
    }

    refreshUploadFiles = async () => {
        try {
            const { uploadDir } = this.state;
            if (!uploadDir) {
                Alert.alert('提示', '请先启动服务器，再刷新文件列表');
                return;
            }

            console.log('Checking uploadDir:', uploadDir);

            const exists = await RNFS.exists(uploadDir);
            if (!exists) {
                this.setState({ uploadFiles: [] });
                Alert.alert('提示', 'uploadDir 不存在');
                return;
            }

            const items = await RNFS.readDir(uploadDir);
            const files = items
                .filter(it => it.isFile())
                .map(it => ({
                    name: it.name,
                    path: it.path,
                    size: Number(it.size || 0),
                }))
                .sort((a, b) => b.size - a.size);

            this.setState({ uploadFiles: files });
        }
        catch (e) {
            Alert.alert('刷新失败', String(e));
        }
    }

    deleteUploadFile = async (file) => {
        Alert.alert('确认删除', `是否删除 ${file.name} ?`, [
            { text: '取消', style: 'cancel' },
            {
                text: '删除',
                style: 'destructive',
                onPress: async () => {
                    try {
                        await RNFS.unlink(file.path);
                        await this.refreshUploadFiles();
                    }
                    catch (e) {
                        Alert.alert('删除失败', String(e));
                    }
                },
            },
        ]);
    }

    saveAsUploadFile = async (file) => {
        try {
            if (Platform.OS === 'android' && NativeModules?.MyNativeModule?.saveAsToPickedDirectory) {
                const retStr = await NativeModules.MyNativeModule.saveAsToPickedDirectory(file.path, file.name);
                const ret = JSON.parse(retStr || '{}');
                if (ret.ret === 'ok') {
                    Alert.alert('另存成功', `${ret.name || file.name}\n${this.formatFileSize(ret.size || 0)}`);
                }
                else if (ret.ret === 'cancel') {
                    // 用户取消
                }
                else {
                    Alert.alert('另存失败', ret.msg || ret.ret || retStr || '未知错误');
                }
                return;
            }

            const rootDir = Platform.OS === 'android' ? RNFS.DownloadDirectoryPath : RNFS.DocumentDirectoryPath;
            const saveDir = `${rootDir}/RNTest5_saved`;
            const dirExists = await RNFS.exists(saveDir);
            if (!dirExists) {
                await RNFS.mkdir(saveDir);
            }

            const dot = file.name.lastIndexOf('.');
            const base = dot > 0 ? file.name.substring(0, dot) : file.name;
            const ext = dot > 0 ? file.name.substring(dot) : '';
            let destPath = `${saveDir}/${file.name}`;
            let idx = 1;
            while (await RNFS.exists(destPath)) {
                destPath = `${saveDir}/${base}_${idx}${ext}`;
                idx++;
            }

            await RNFS.copyFile(file.path, destPath);
            Alert.alert('另存成功', destPath);
        }
        catch (e) {
            Alert.alert('另存失败', String(e));
        }
    }

    copyDownloadLink = (file) => {
        const { selectedIp, port } = this.state;
        if (!selectedIp || !port) {
            Alert.alert('提示', '请先设置服务器地址');
            return;
        }
        const link = `http://${selectedIp}:${port}/download?${encodeURIComponent(file.name)}`;
        Clipboard.setString(link);
        Alert.alert('已复制下载链接', link);
    }

    openDownloadLinkQrCode = (file) => {
        const { selectedIp, port } = this.state;
        if (!selectedIp || !port) {
            Alert.alert('提示', '请先设置服务器地址');
            return;
        }
        const link = `http://${selectedIp}:${port}/download?${encodeURIComponent(file.name)}`;
        this.setState({
            showQrCode: true,
            qrTitle: `下载链接二维码`,
            qrValue: link,
        });
    }

    importExternalFile = async () => {
        try {
            const { uploadDir } = this.state;
            if (!uploadDir) {
                Alert.alert('提示', '请先启动服务器');
                return;
            }

            if (!NativeModules?.MyNativeModule?.pickAndImportFile) {
                Alert.alert('提示', '当前版本不支持导入外部文件');
                return;
            }

            const retStr = await NativeModules.MyNativeModule.pickAndImportFile(uploadDir);
            const ret = JSON.parse(retStr || '{}');
            if (ret.ret === 'ok') {
                await this.refreshUploadFiles();
                Alert.alert('导入成功', `${ret.name || ''}\n${this.formatFileSize(ret.size || 0)}`);
            }
            else if (ret.ret === 'cancel') {
                // 用户取消，不提示
            }
            else {
                Alert.alert('导入失败', ret.msg || ret.ret || retStr || '未知错误');
            }
        }
        catch (e) {
            Alert.alert('导入失败', String(e));
        }
    }

    importExternalFiles = async () => {
        try {
            const { uploadDir } = this.state;
            if (!uploadDir) {
                Alert.alert('提示', '请先启动服务器');
                return;
            }

            if (!NativeModules?.MyNativeModule?.pickAndImportFiles) {
                Alert.alert('提示', '当前版本不支持批量导入');
                return;
            }

            const retStr = await NativeModules.MyNativeModule.pickAndImportFiles(uploadDir);
            const ret = JSON.parse(retStr || '{}');
            if (ret.ret === 'ok') {
                await this.refreshUploadFiles();
                Alert.alert('批量导入成功', `已导入 ${ret.importedCount || 0} 个文件`);
            }
            else if (ret.ret === 'cancel') {
                // 用户取消
            }
            else {
                Alert.alert('批量导入失败', ret.msg || ret.ret || retStr || '未知错误');
            }
        }
        catch (e) {
            Alert.alert('批量导入失败', String(e));
        }
    }

    renderUploadItem = ({ item }) => {
        return (
            <View style={{backgroundColor:'#fff', borderWidth:1, borderColor:'#e3e3e3', borderRadius:8, padding:10, marginBottom:8}}>
                <Text style={{fontSize:14, color:'#222'}} numberOfLines={1}>{item.name}</Text>
                <Text style={{fontSize:12, color:'#666', marginTop:4}}>{this.formatFileSize(item.size)}</Text>
                <View style={{flexDirection:'row', marginTop:8}}>
                    <TouchableOpacity
                        onPress={() => this.deleteUploadFile(item)}
                        style={{backgroundColor:'#d32f2f', borderRadius:6, paddingHorizontal:10, paddingVertical:5, marginRight:8}}
                    >
                        <Text style={{color:'#fff', fontSize:12}}>删除</Text>
                    </TouchableOpacity>
                    <TouchableOpacity
                        onPress={() => this.saveAsUploadFile(item)}
                        style={{backgroundColor:'#455a64', borderRadius:6, paddingHorizontal:10, paddingVertical:5, marginRight:8}}
                    >
                        <Text style={{color:'#fff', fontSize:12}}>另存</Text>
                    </TouchableOpacity>
                    <TouchableOpacity
                        onPress={() => this.copyDownloadLink(item)}
                        style={{backgroundColor:'#1976d2', borderRadius:6, paddingHorizontal:10, paddingVertical:5, marginRight:8}}
                    >
                        <Text style={{color:'#fff', fontSize:12}}>下载链接复制</Text>
                    </TouchableOpacity>
                    <TouchableOpacity
                        onPress={() => this.openDownloadLinkQrCode(item)}
                        style={{backgroundColor:'#2e7d32', borderRadius:6, paddingHorizontal:10, paddingVertical:5}}
                    >
                        <Text style={{color:'#fff', fontSize:12}}>下载链接二维码</Text>
                    </TouchableOpacity>
                </View>
            </View>
        );
    }

    render() {
        const { ipOptions, selectedIp, port, showQrCode, qrTitle, qrValue, webServerRunning, uploadFiles } = this.state;
        const serverAddress = selectedIp ? `http://${selectedIp}:${port || ''}` : '';

        return (
            <View style={{height:ScreenUtil.flexHeight, width:ScreenUtil.deviceWidth, backgroundColor:'#f5f5f5', padding:12 }}>
                <Text style={{fontSize:18, fontWeight:'600', marginBottom:10}}>服务器地址设置</Text>

                <Text style={{fontSize:14, marginBottom:6}}>选择IP</Text>
                <SelectList
                    setSelected={(val) => this.setState({selectedIp: val})}
                    data={ipOptions}
                    save="value"
                    placeholder="请选择IP"
                    boxStyles={{backgroundColor:'#fff', borderColor:'#ddd'}}
                    dropdownStyles={{backgroundColor:'#fff', borderColor:'#ddd'}}
                />

                <Text style={{fontSize:14, marginTop:12, marginBottom:6}}>端口</Text>
                <TextInput
                    value={port}
                    onChangeText={(v) => this.setState({port: v.replace(/[^0-9]/g, '')})}
                    keyboardType="number-pad"
                    placeholder="例如 8899"
                    style={{backgroundColor:'#fff', borderWidth:1, borderColor:'#ddd', borderRadius:8, paddingHorizontal:10, paddingVertical:8}}
                />

                <View style={{flexDirection:'row', alignItems:'center', marginTop:12}}>
                    <Text style={{fontSize:14, color:'#333'}}>服务器地址：</Text>
                    <TouchableOpacity
                        onPress={() => this.copyServerAddress(serverAddress)}
                        style={{marginLeft:10, backgroundColor:'#1976d2', borderRadius:6, paddingHorizontal:10, paddingVertical:5}}
                    >
                        <Text style={{color:'#fff', fontSize:12}}>复制</Text>
                    </TouchableOpacity>
                    <TouchableOpacity
                        onPress={() => this.openQrCode(serverAddress)}
                        style={{marginLeft:8, backgroundColor:'#2e7d32', borderRadius:6, paddingHorizontal:10, paddingVertical:5}}
                    >
                        <Text style={{color:'#fff', fontSize:12}}>二维码</Text>
                    </TouchableOpacity>
                </View>
                <Text style={{fontSize:16, color:'#1976d2', marginTop:4}}>{serverAddress || '请先选择IP并填写端口'}</Text>

                <View style={{flexDirection:'row', marginTop:12, alignItems:'center'}}>
                    {!webServerRunning ? (
                        <TouchableOpacity
                            onPress={this.startWebServer}
                            style={{backgroundColor:'#1565c0', borderRadius:6, paddingHorizontal:14, paddingVertical:8}}
                        >
                            <Text style={{color:'#fff', fontSize:13}}>启动服务器</Text>
                        </TouchableOpacity>
                    ) : (
                        <TouchableOpacity
                            onPress={this.stopWebServer}
                            style={{backgroundColor:'#c62828', borderRadius:6, paddingHorizontal:14, paddingVertical:8}}
                        >
                            <Text style={{color:'#fff', fontSize:13}}>停止服务器</Text>
                        </TouchableOpacity>
                    )}
                    <Text style={{marginLeft:10, alignSelf:'center', color:webServerRunning ? '#2e7d32' : '#666'}}>
                        {webServerRunning ? '运行中' : '未启动'}
                    </Text>
                </View>

                <View style={{marginTop:14, flex:1}}>
                    <View style={{flexDirection:'row', alignItems:'center', justifyContent:'center', backgroundColor:'lightgray', marginBottom:10}}>
                        <Text style={{fontSize:15, fontWeight:'600', color:'#333', marginRight:10}}>uploadDir 文件列表</Text>
                        
                    </View>
                    <View style={{flexDirection:'row', marginBottom:10}}>
                        <TouchableOpacity
                            onPress={this.importExternalFile}
                            style={{backgroundColor:'#5d4037', borderRadius:6, paddingHorizontal:14, paddingVertical:8, marginRight:8}}
                        >
                            <Text style={{color:'#fff', fontSize:13}}>添加外部文件</Text>
                        </TouchableOpacity>
                        <TouchableOpacity
                            onPress={this.importExternalFiles}
                            style={{backgroundColor:'#795548', borderRadius:6, paddingHorizontal:14, paddingVertical:8, marginRight:8}}
                        >
                            <Text style={{color:'#fff', fontSize:13}}>批量导入</Text>
                        </TouchableOpacity>
                        <TouchableOpacity
                            onPress={this.refreshUploadFiles}
                            style={{backgroundColor:'#6d4c41', borderRadius:6, paddingHorizontal:14, paddingVertical:8, marginRight:8}}
                        >
                            <Text style={{color:'#fff', fontSize:13}}>刷新</Text>
                        </TouchableOpacity>
                    </View>
                    <FlatList
                        data={uploadFiles}
                        keyExtractor={(item) => item.path}
                        renderItem={this.renderUploadItem}
                        ListEmptyComponent={<Text style={{fontSize:12, color:'#666'}}>暂无文件，点击“刷新”加载</Text>}
                    />
                </View>

                <Modal
                    visible={showQrCode}
                    transparent={true}
                    animationType="fade"
                    onRequestClose={() => this.setState({showQrCode:false})}
                >
                    <View style={{flex:1, backgroundColor:'rgba(0,0,0,0.45)', justifyContent:'center', alignItems:'center', padding:24}}>
                        <View style={{backgroundColor:'#fff', borderRadius:10, padding:16, alignItems:'center', width:'90%'}}>
                            <Text style={{fontSize:16, fontWeight:'600', marginBottom:12}}>{qrTitle}</Text>
                            {qrValue ? (
                                <QRCode value={qrValue} size={220} />
                            ) : (
                                <Text style={{color:'#666'}}>地址为空</Text>
                            )}
                            <Text style={{fontSize:12, color:'#444', marginTop:12, textAlign:'center'}}>{qrValue}</Text>
                            <TouchableOpacity
                                onPress={() => this.setState({showQrCode:false, qrValue:''})}
                                style={{marginTop:14, backgroundColor:'#555', borderRadius:6, paddingHorizontal:16, paddingVertical:8}}
                            >
                                <Text style={{color:'#fff'}}>关闭</Text>
                            </TouchableOpacity>
                        </View>
                    </View>
                </Modal>
            </View>
        );
    }
}
