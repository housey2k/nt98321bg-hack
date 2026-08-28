class Universal extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "Universal-CS8x"
		
	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"
	
	CommandLen = 11

	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]
	
	// 地址码
	RS485Addr = 0xFF
	
	// 是否有弹起消息
	UpMsg = 0
	
	// 是否通过脚本解析
	ParseMode = 1
	
	// 命令码
	KEYOperator = {
				//FF 01 FF FF CMD1 CMD2
				KEYZERO = [0xFF, 0x09, KEYVALUE.KEY_0],
				KEYONE = [0xFF, 0x00, KEYVALUE.KEY_1],
				KEYTWO = [0xFF, 0x01, KEYVALUE.KEY_2],
				KEYTHREE = [0xFF, 0x02, KEYVALUE.KEY_3],
				KEYFOUR = [0xFF, 0x03, KEYVALUE.KEY_4], 
				KEYFIVE = [0xFF, 0x04, KEYVALUE.KEY_5],
				KEYSIX = [0xFF, 0x05, KEYVALUE.KEY_6],
				KEYSEVEN = [0xFF, 0x06, KEYVALUE.KEY_7],
				KEYEIGHT = [0xFF, 0x07, KEYVALUE.KEY_8],
				KEYNINE = [0xFF, 0x08, KEYVALUE.KEY_9],
				KEYTELE = [0x0C, 0xFF, KEYVALUE.KEY_TELE],
				KEYWIDE = [0x0D, 0xFF, KEYVALUE.KEY_WIDE],
				KEYCLEAR = [0xFF, 0x0B, KEYVALUE.KEY_ESC],
				KEYMENU = [0x0E, 0xFF, KEYVALUE.KEY_MENU],
				KEYENTER = [0xFF, 0x0A, KEYVALUE.KEY_RET],
				KEYSCAN = [0x00, 0xFF, KEYVALUE.KEY_SPLIT],
				KEYTOUR = [0x01, 0xFF, KEYVALUE.KEY_SPLIT1],
				KEYPRESET = [0x02, 0xFF, KEYVALUE.KEY_SPLIT4],
				KEYPFAST = [0x03, 0xFF, KEYVALUE.KEY_SPLIT4],
				KEYSLOW = [0x05, 0xFF, KEYVALUE.KEY_SPLIT16],
				KEYNORMAL = [0x04, 0xFF, KEYVALUE.KEY_SPLIT9],
				KEYPALARM = [0x06, 0xFF, KEYVALUE.KEY_PTZ],
				KEYCPRESET = [0x07, 0xFF, KEYVALUE.KEY_UP],
				KEYSPRESET = [0x08, 0xFF, KEYVALUE.KEY_UNKNOWN],
				KEYRECPATTERN = [0x09, 0xFF, KEYVALUE.KEY_LEFT],
				KEYSTOPRECORD = [0x0A, 0xFF, KEYVALUE.KEY_DOWN],
				KEYPATTERN = [0x0B, 0xFF, KEYVALUE.KEY_RIGHT],
				
				//CMD1 CMD2 VER HOR 00 00
				KEYUP = [0xE0, 0xE8, KEYVALUE.KEY_UP],
				KEYDOWN = [0xE0, 0xF0, KEYVALUE.KEY_DOWN],
				KEYLEFT = [0xE0, 0xE2, KEYVALUE.KEY_RIGHT],
				KEYRIGHT = [0xE0, 0xE4, KEYVALUE.KEY_LEFT],
				
				//CMD1 CMD2 00 00 00 00
				KEYIRISOPEN = [0xFF, 0xF0, KEYVALUE.KEY_IRIS_OPEN],
				KEYIRISCLOSE = [0xFF, 0xE8, KEYVALUE.KEY_IRIS_CLOSE],
				KEYFOCUSFAR = [0xFF, 0xE1, KEYVALUE.KEY_FOCUS_FAR],
				KEYFOCUSNEAR = [0xFF, 0xE2, KEYVALUE.KEY_FOCUS_NEAR],
				}
				
	function setAddr(addr)
	{
		RS485Addr = addr;
	}
	
	function CheckData(cmdBuf)
	{
		local addr = cmdBuf[1];
		if (cmdBuf[0] != HeadData[0])
		{
			::print("cmdBuf: "+cmdBuf[0]+" HeadData: "+HeadData[0]+"\n");
			return -1;
		}
		if (addr != RS485Addr)
		{
			::print("addr: "+addr+" RS485Addr: "+RS485Addr+"\n");
			return -1;
		}
		
		return 0;
	}
	
	function ParseData(cmdBuf)
	{
		local ret = 0;
	
		ret = CheckData(cmdBuf);
		if (ret == 0)
		{
			foreach(k,v in KEYOperator)
			{
				local tmp = KEYOperator.rawget(k);
				////FF 01 FF FF CMD1 CMD2
				if (cmdBuf[3] == 0xFF && cmdBuf[4] == 0x01 && cmdBuf[5] == 0xFF && cmdBuf[6] == 0xFF)
				{
					if (tmp[0] == cmdBuf[7] && tmp[1] == cmdBuf[8])
					{
						return tmp[2];
					}
				}
				//CMD1 CMD2 xx xx xx xx
				else if(tmp[0] == cmdBuf[3] && tmp[1] == cmdBuf[4])
				{
					return tmp[2];
				}
			}
			return KEYVALUE.KEY_UNKNOWN;
		}
		else
		{
			return KEYVALUE.KEY_UNKNOWN;
		}
	}
}

local cUniversal = Universal();

return cUniversal;