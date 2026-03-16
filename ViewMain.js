import React, { Component } from 'react';
import { View, TouchableOpacity, Text, NativeModules, Platform, FlatList, RefreshControl, ScrollView, TextInput, Switch, Image,
    DeviceEventEmitter, Keyboard, Alert, findNodeHandle, Modal } from 'react-native';
import PropTypes from 'prop-types';
import { SelectList } from 'react-native-dropdown-select-list';
import Clipboard from '@react-native-clipboard/clipboard';
import QRCode from 'react-native-qrcode-svg';

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
            webServerRunning: false,
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
                this.setState({ webServerRunning: true });
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
        this.setState({ showQrCode: true });
    }

    render() {
        const { ipOptions, selectedIp, port, showQrCode, webServerRunning } = this.state;
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

                <View style={{flexDirection:'row', marginTop:12}}>
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

                <Modal
                    visible={showQrCode}
                    transparent={true}
                    animationType="fade"
                    onRequestClose={() => this.setState({showQrCode:false})}
                >
                    <View style={{flex:1, backgroundColor:'rgba(0,0,0,0.45)', justifyContent:'center', alignItems:'center', padding:24}}>
                        <View style={{backgroundColor:'#fff', borderRadius:10, padding:16, alignItems:'center', width:'90%'}}>
                            <Text style={{fontSize:16, fontWeight:'600', marginBottom:12}}>服务器地址二维码</Text>
                            {serverAddress ? (
                                <QRCode value={serverAddress} size={220} />
                            ) : (
                                <Text style={{color:'#666'}}>地址为空</Text>
                            )}
                            <Text style={{fontSize:12, color:'#444', marginTop:12, textAlign:'center'}}>{serverAddress}</Text>
                            <TouchableOpacity
                                onPress={() => this.setState({showQrCode:false})}
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
