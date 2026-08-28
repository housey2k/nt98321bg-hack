//# sourceURL=main.js
var BrowseType = {
	BrowseMSIE:0,
	BrowseSafari:1,
	BrowseChrome:2,
	BrowseMacOS:3,
	BrowseFirefox:4,
	BrowseEdge:5,
	BrowseOpera:6,
	BrowseMozilla:7,
	Browse360:8,
	BrowseQQ:9,
	BrowseBaidu:10,
	BrowseSougou:11,
	BrowseUnknow:12
}

var gVar;
var gDevice;
var lg;
var gOemInfo;
var ColorSet = "#7EA264";
var gCloseFlag = 0;
var g_loginTimeout = 15;
var g_intervalID = -1;
var g_defaultStreamType = 1;
var g_videomovetime = -1;
var ws = null;
var g_bAlarm = false;
var g_Radio = 1.25;
var g_keepAliveID = -1;
var g_bReConnect = false;
var nKeepAliveCount=0;
var g_bKeepAlive = true;
var g_bDisconneting = false;
var g_bReConnecting = false;
var g_BrowseType = BrowseType.BrowseUnknow;
var g_browserVer;
var g_bEncryptVideo = false;
var g_bLimitBirate = false;
var g_bLoadOcx = false;
var g_Ocx = null;
var g_RecPath = "";
var g_CapPath = "";
var g_DownPath = "";
var g_bAutoPlay = true;
var g_nOpenVideoMode = 1;
var g_bLoadPlugin = !1;
var g_PluginPort=54455;
var gNet = null;
var gBrowseCtrl = null;

function SendMsgToWeb(a) {
	VideoOcxEventCallBack(a);
}
function VideoOcxEventCallBack(){}
function previewEventCallBack() {}
function playbackEventCallBack() {}
function timeLineResizeCallBack() {}
function AlarmInfoEventCallBack() {}
function UpgradeEventCallBack(){}
function OnlineUpgradeEventCallBack(){}
function ClientConfigEventCallBack(){}
function initColorSetEvent(){}
function RemainTimeCallback(){}
function StopVoiceRecord(){}
function EndSendFile(){}
function GetHtml(b) {
	var c = $.extend({
		webUrl: "",
		callback: function(d) {}
	}, b);
	if(g_b8M){
		c.webUrl = "http://127.0.0.1:" + g_PluginPort;
		if(g_WebStyle != ""){
			c.webUrl +="/WebStyle_"+ g_WebStyle;
		}
		c.webUrl +="/"+b.webUrl;
	}
	$.get(c.webUrl+"?version="+version_web, "", c.callback, "html")
};
function GetJS(b) {
	var c = $.extend({
		webUrl: "",
		callback: function(d) {}
	}, b);
	if(g_b8M){
		c.webUrl = "http://127.0.0.1:" + g_PluginPort;
		if(g_WebStyle != ""){
			c.webUrl +="/WebStyle_"+ g_WebStyle;
		}
		c.webUrl +="/"+b.webUrl;
	}
	$.getScript(c.webUrl+"?version="+version_web, c.callback)
};
function CreateImageHttp(port, t){
	var n = new Image;
	n.crossOrigin='anonymous';
	n.onload = function() {
		t.success && t.success(port)
	}, n.onerror = function() {
		t.error && t.error()
	}, n.onabort = function() {
		t.abort && t.abort()
	};
	var i = "http://127.0.0.1:" + port + "/Cfg/tip.png?update=" +t.timeStamp;
	n.src = i;
}
function detectPort(s, e, callback){
	var o = [];
	for (var a = s; a <= e; a++) o.push(a);
	var u = (new Date).getTime();
	var s = 0;
	var l = function(t) {
		setTimeout((function() {
			CreateImageHttp(o[t], {
				timeStamp: u + t,
				success: function(g) {
					callback(g);
				},
				error: function() {
					s++;
					if(o.length === s){
						callback(-1);
					}
				}
			})
		}), 100)
	}
	for(var i =0; i < o.length;i++){
		l(i);
	}
}
function checkPluginExist(callback){
	detectPort(54455,54465,function(port){
		if(port < 0){
			if(g_BrowseType == BrowseType.BrowseMSIE
				|| g_BrowseType == BrowseType.BrowseEdge){
					$("#runDiv").click(function(event){
						window.protocolCheck($(this).attr("href"),function(){
							callback(-1);
						});
						event.preventDefault ? event.preventDefault() : event.returnValue = false;
					});
					$("#runDiv").click();
			}else{
				var eleLink = document.createElement('a');
				eleLink.style.display = 'none';
				eleLink.href = "VideoPlayTool://1";
				document.body.appendChild(eleLink);
				eleLink.click();
			}
			setTimeout(function(){
				detectPort(54455,54465,function(port){
					callback(port);
				});
			}, 1000);
		}else{
			callback(port);
		}
	});
}
function GetPluginVersion(b){
	if(g_nOpenVideoMode == 1){
		var req = {"MainType":1,"SubType":0, "WebStyle":g_WebStyle};
		XMLHttp.sendReq('POST', "http://127.0.0.1:"+g_PluginPort+"/Cmd-WebLocalCtrl", JSON.stringify(req), function(e){
			DebugStringEvent(JSON.stringify(e));
			b(e);
		});
	}
}
function compareVersion(a, f) {
	var d = a.split(".");
	var e = f.split(".");
	if (d.length != e.length) {
		return false;
	}
	for (var b = 0; b < d.length; ++b) {
		if (d[b] * 1 < e[b] * 1) {
			return false;
		} else {
			if (d[b] * 1 > e[b] * 1) {
				return true;
			}
		}
	}
	return true;
}
function DebugStringEvent(a) {
	if (window.console) {
		window.console.log("PluginDebugInfo:" + a)
	}
}
function ShowErrorTip(nType,version){
	var lang = navigator.language||navigator.userLanguage;
	lang = lang.substr(0, 2);
	var tip= "",tip1="",tip2="",tip3="",tip4="";
	if(nType == 1 || nType == 5){
		if(nType == 1){
			tip="Please click here to download and install VideoPlayTool";
			tip1= "Notes: If you are still prompted to download and install after the play tool under Google Chrome is installed, please do as follows:";
			tip2="Step 1: Enter in the address bar of Google Chrome:";
			tip3="Step 2: Search &apos;Block insecure private network requests&apos;，Select Disable";
			tip4="Step 3: Close Google Chrome and reopen the webpage";
			if(lang == "zh"){
				tip="请点此，下载并安装播放工具";
				tip1="提示：如果谷歌浏览器下载安装播放工具后，还是提示请下载安装，请按如下操作："; 
				tip2="步骤一： 在谷歌浏览器地址栏输入：";
				tip3="步骤二：搜索 &apos;Block insecure private network requests&apos; 配置项，选择Disable";
				tip4="步骤三：关闭谷歌浏览器重新打开网页。";
			}
			$("#LossOcxTip").html(tip);
			$("#SetupTip1").html(tip1);
			$("#SetupTip2").html(tip2);
			$("#SetupTip3").html(tip3);
			$("#SetupTip4").html(tip4);
			if(g_BrowseType == BrowseType.BrowseChrome){
				$("#OcxTip").css("width","500px");
				$("#OcxTip").css("height","160px");
				$("#GoogleTip").show();
			}else{
				$("#OcxTip").css("width","400px");
				$("#OcxTip").css("height","60px");
				$("#OcxTipRow").css("margin-top", "20px");
			}
		}else{
			$("#OcxTipRow").css("margin-top", "20px");
			$("#OcxTip").css("width","400px");
			$("#OcxTip").css("height","60px");
			tip="Please click here to download and update VideoPlayTool";
			if(lang == "zh"){
				tip="请点此，下载并更新播放工具";
			}
			$("#LossOcxTip").html(tip);
		}
		$("#LossOcxTip").click(function(){
			window.open(downloadAddr);
		});
		$("#OcxTipRow").show();
		$("#webplugins").show();
	}else{
		$("#OcxTip").css("width","400px");
		$("#OcxTip").css("height","60px");
		if(nType == 2){
			tip="Failed to obtain VideoPlayTool version";
			if(lang == "zh"){
				tip="获取播放工具版本失败";
			}
		}else if(nType == 3){
			tip="This browser is not supported";
			if(lang == "zh"){
				tip="暂时不支持该浏览器";
			}
		}else if(nType == 4){
			tip="Fail To Obtain Pre-login Info";
			if(lang == "zh"){
				tip="获取预登录信息失败";
			}
		}else if(nType == 6){
			if(g_BrowseType == BrowseType.BrowseMSIE){
				tip = "Only support browsers above ie10, please update your browser";
				if(lang == "zh"){
					tip="只支持ie10以上的浏览器，请更新浏览器";
				}
			}else{
				tip = "The browser version is too low, please update to at least "+version;
				if(lang == "zh"){
					tip="浏览器版本太低，请更新到"+version+"版本以上";
				}
			}
		}
		$("#ErrorTip").html(tip);
		$("#ErrTipRow").show();
		$("#webplugins").show();
	}
	$("#OcxTip").show();
}
$(function() {
	var a = navigator.userAgent.toLowerCase();
	if (!$.browser) {
		$.browser = {}
	}
	g_browserVer = (a.match(/.+(?:rv|it|ra|ie)[\/: ]([\d.]+)/) || [0, "0"])[1];
	try {
		if (navigator.platform.toLowerCase().indexOf("mac") > -1) {
			g_BrowseType = BrowseType.BrowseMacOS;
		}
		if ("ActiveXObject" in window) {
			g_BrowseType = BrowseType.BrowseMSIE;
		}else if(a.indexOf("qqbrowser") > 0){
			g_BrowseType=BrowseType.BrowseQQ;
		} else if(a.indexOf("bidubrowser") > 0){
			g_BrowseType=BrowseType.BrowseBaidu;
		}else if(a.indexOf("metasr") > 0){
			g_BrowseType=BrowseType.BrowseSougou;
		}else if (/opera/.test(a)||a.indexOf("opr") > 0) {
			g_BrowseType = BrowseType.BrowseOpera;
		} else {
			if (a.indexOf("edge") > 0) {
				g_BrowseType = BrowseType.BrowseEdge;
			} else {
				if (a.indexOf("safari") > 0 && a.indexOf("chrome") < 0) {
					g_BrowseType = BrowseType.BrowseSafari;
					g_browserVer = a.match(/version\/([\d.]+).*safari/)[1]
				} else {
					if (a.indexOf("firefox") > 0) {
						g_BrowseType = BrowseType.BrowseFirefox;
					} else {
						if (a.indexOf("chrome") > 0) {
							g_BrowseType = BrowseType.BrowseChrome;
							g_browserVer = a.match(/chrome\/([\d.]+)/)[1]
						} else {
								if (/mozilla/.test(a) && !/(compatible|webkit)/.test(a)) {
									g_BrowseType = BrowseType.BrowseMozilla;
								} else {
									g_BrowseType = BrowseType.BrowseMSIE;
								}
						}
					}
				}
			}
		}
	} catch (b) {}
	$(".mcmcmain").html('<div id="ipcplugin"></div>');
	$(document).keydown(function(c) {
		c = c || window.event;
		if (!c.srcElement) {
			c.srcElement = c.target
		}
		if (c.keyCode == 8 && ((c.srcElement.readOnly == null || c.srcElement.readOnly == true) || (c.srcElement.type !=
				"text" && c.srcElement.type != "textarea" && c.srcElement.type != "password"))) {
			c.keyCode = 0;
			c.returnValue = false;
			return false
		}
	});
	if(g_BrowseType == BrowseType.BrowseChrome){
		if(g_browserVer.split(".")[0] * 1 >= 23){
			InitWeb();
		}else{
			ShowErrorTip(6, 23);
		}
	}else if(g_BrowseType == BrowseType.BrowseFirefox){
		if(g_browserVer* 1 >= 52){
			InitWeb();
		}else{
			ShowErrorTip(6, 52);
		}
	}else if(g_BrowseType == BrowseType.BrowseSafari){
		ShowErrorTip(3);
	}else if(g_BrowseType == BrowseType.BrowseMSIE){
		if(g_browserVer * 1 >= 10){
			InitWeb();
		}else{
			ShowErrorTip(6, 10);
		}
	}else if(g_BrowseType == BrowseType.BrowseOpera ||
		g_BrowseType == BrowseType.BrowseQQ){
		InitWeb();
	}else{
		ShowErrorTip(3);
	}
});

function InitWeb() {
	$("#live", "#login", "#LiveMenu", "#playback", "#PlayBackMenu", "#ClientMenu").css("display", "block");
	$("body,#userName,#loginPsw").keydown(function(f) {
		if (f.keyCode == 13) {
			if ($("#login_user_prompt").css("display") != "none") {
				$("#login_btn_user_ok").click()
			}
		}
	});
	$("body").on("mousedown mouseup", ".psw_eye_arc", function() {
		var b = $(this).siblings("input.PswEyeShow").eq(0);
		if (b.attr("type") == "password") {
			b.attr("type", "text")
		} else {
			b.attr("type", "password")
		}
	});
	$("body").on("focus keyup", ".PswEyeShow", function() {
		var b = $(this);
		$(".psw_eye_arc").css("display", "none");
		if ($(this).val() != "") {
			b.next().css("display", "block")
		} else {
			b.next().css("display", "none")
		}
		$(".PswEyeShow").mouseout(function() {
			$(this).attr("type", "password")
		})
	});
	$("body").bind("click", function(b) {
		var c = b.target;
		if (!($(c).hasClass("psw_eye_arc") && $(c).is(":visible"))) {
			if (!$(c).next().hasClass("psw_eye_arc")) {
				$(".psw_eye_arc").css("display", "none")
			}
		}
	});
	$("body").on("click", ".btn_cancle,.second_close", function() {
		if(this.id=="AudioDLgClose"){
			return;
		}
		$("#SecondaryContent, #ForgetPwdContent, .dialog_role").css("display", "none");
		MasklayerHide();
	})
	$("#logoutMenu").mouseover(function(){
		$("#Logout").css("background-position", "-24px -24px");
	}).mouseout(function(){
		$("#Logout").css("background-position", "-0 -24px");
	}).click(function(){
		closewnd(1);
	});
	
	function InitLanguage(a) {
		if (gOemInfo.langues.indexOf(a) == -1) {
			gOemInfo.defaultLg = "English";
		}else{
			gOemInfo.defaultLg = a;
		}
		var c = gOemInfo.langues.split(" ");
		for (var b = 0; b < c.length; b++) {
			gOemInfo.mul[b] = new Array;
			for (var a = 0; a < LanguageArray.length; a++) {
				if (c[b] == LanguageArray[a][0]) {
					gOemInfo.mul[b][0] = LanguageArray[a][0];
					gOemInfo.mul[b][1] = LanguageArray[a][1];
					break;
				}
			}
		}
		gVar.lg = $.cookie("Language");
		if (gVar.lg == null) {
			gVar.lg = gOemInfo.defaultLg;
		}
		var b;
		for (b = 0; b < gOemInfo.mul.length; b++) {
			if (gOemInfo.mul[b][0] == gVar.lg) {
				break
			}
		}
		if (b >= gOemInfo.mul.length) {
			gVar.lg = gOemInfo.defaultLg;
		}
	}
	function GetPreDevInfo(){
		gOemInfo = new OemInfo();
		gDevice = new DeviceInfo();
		gVar = new GlobalVar();
		lg = new HashmapCom();
		if(g_WebStyle != "" && g_WebStyle != undefined){
		    gOemInfo.skin = g_WebStyle;
		}else{
		    gOemInfo.skin = "general";
		}
		gOemInfo.langues="English SimpChinese";
		gOemInfo.defaultLg="English";
		var href = window.location.href;
		gDevice.ip = href.split("//")[1].split("/")[0];
		if (gDevice.ip.indexOf(":") != -1) {
			gDevice.httpPort = gDevice.ip.split(":")[1];
			gDevice.ip = gDevice.ip.split(":")[0]
		}else{
			gDevice.httpPort = 80;
		}
		var bInit = gDevice.Init($("#ipcplugin"));
		var c = document.getElementsByTagName("body")[0];
		c.setAttribute("onunload", "closewnd(0)");
		c.setAttribute("onselectstart", "return forbidSelect();");
		c.setAttribute("onload", "window.onresize();");
		if(!bInit){
			alert("Plugin is not loaded!");
			return;
		}
		var c = document.getElementsByTagName("head")[0];
		var d = document.createElement("link");
		if(g_b8M){
			d.href = "http://127.0.0.1:"+g_PluginPort;
			if(g_WebStyle != ""){
				d.href += "/WebStyle_"+g_WebStyle;
			}
			d.href +="/images_"+gOemInfo.skin+"/skin.css";
		}else{
			d.href = "/images_"+gOemInfo.skin+"/skin.css";
		}
		d.rel = "stylesheet";
		d.type = "text/css";
		c.appendChild(d);
		gDevice.GetPreLoginInfo(function(a){
			if(a.Ret == 100){
				gDevice.tcpPort=a.TCPPort;
				InitLanguage(a.Language);
				var req = {"Name": "GetSafetyAbility"};
				RfParamCall(function (b, c){
					if(b.Ret == 100){
						gVar.SafetyAbility = b;
					}else{
						gVar.SafetyAbility = null;
					}
					gVar.ChangeLang(gVar.lg);
				}, "Prompt", "GetSafetyAbility", -1, WSMsgID.WsMsgID_AUTHORIZATION, req);
			}else{
				ShowErrorTip(4);
			}
		});
	}
	function loadJS(){
		GetJS({
			webUrl:"plugin/common.js",
			callback:function(){
				GetJS({
					webUrl:"plugin/function.js",
					callback:function(){
						GetJS({
							webUrl:"plugin/language.js",
							callback:function(){
								GetJS({
									webUrl:"plugin/RSUI.js",
									callback:function(){
										GetJS({
											webUrl:"plugin/class.js",
											callback:function(){
												GetPreDevInfo();
											}
										});
									}
								});
							}
						});
					}
				});
			}
		});
	}
	checkPluginExist(function(port){
		if(port > 0){
			g_PluginPort = port;
			var m = "0.0.0.0";
			GetPluginVersion(function(e){
				if(e.Ret == 100){
					m = e.Version;
					if(compareVersion(m, version_msie)){
						loadJS();
					}else{
						g_nOpenVideoMode = 0;
						ShowErrorTip(5);
					}
				}else{
					ShowErrorTip(2);
				}
			});
		}else{
			g_nOpenVideoMode = 0;
			ShowErrorTip(1);
		}
	});
}
