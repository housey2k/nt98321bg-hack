var XMLHttp = {
    _objPool: [],
    _getInstance: function (){
        for (var i = 0; i < this._objPool.length; i ++){
            if (this._objPool[i].xhr.readyState == 0 || this._objPool[i].xhr.readyState == 4){
                return this._objPool[i];
            }
        }
        var len = this._objPool.length;
        this._objPool[len] = {};
        this._objPool[len].xhr = this._createObj();
        this._objPool[len].timer = null;
        return this._objPool[len];
    },
    _createObj: function (){
        if (window.XMLHttpRequest){
            var objXMLHttp = new XMLHttpRequest();
        }else{
            var MSXML = ['MSXML2.XMLHTTP.5.0', 'MSXML2.XMLHTTP.4.0', 'MSXML2.XMLHTTP.3.0', 'MSXML2.XMLHTTP', 'Microsoft.XMLHTTP'];
            for(var n = 0; n < MSXML.length; n ++){
                try{
                    var objXMLHttp = new ActiveXObject(MSXML[n]);        
                    break;
                }catch(e){
                }
            }
        }
        if (objXMLHttp.readyState == null){
            objXMLHttp.readyState = 0;
            objXMLHttp.addEventListener("load", function (){
                objXMLHttp.readyState = 4;
                if (typeof objXMLHttp.onreadystatechange == "function"){
                    objXMLHttp.onreadystatechange();
                }
            },  false);
        }
        return objXMLHttp;
    },
    sendReq: function (method, url, data, callback){
        var obj = this._getInstance();
        var objXMLHttp = obj.xhr;
        if(objXMLHttp){
            function connectFail(){
                if(objXMLHttp.readyState != 0 && objXMLHttp.readyState != 4){
                    objXMLHttp.abort();
                    DebugStringEvent("timeout");
                }
            }
            try{
                if (url.indexOf("?") > 0){
                    url += "&randnum=" + Math.random();
                }else{
                    url += "?randnum=" + Math.random();
                }
                objXMLHttp.abort();
                if (obj.timer) clearTimeout(obj.timer);

                objXMLHttp.open(method, url, true);
                objXMLHttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');
                objXMLHttp.send(data);
                objXMLHttp.onreadystatechange = function (){                  
                    if (objXMLHttp.readyState == 4){
                        if(objXMLHttp.status == 200){
                           // DebugStringEvent(objXMLHttp.responseText);
                            var c = JSON.parse(objXMLHttp.responseText);
                            callback(c);
                        }
                        else{
                            DebugStringEvent(objXMLHttp.status);
                            callback({"Ret":-1});
                        }
                    }
                }
                obj.timer = setTimeout(connectFail, 8000);
            }catch(e){
                DebugStringEvent("expection");
                callback({"Ret":-1});
            }
        }
    }
};