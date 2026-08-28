class LongYang extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "LongYang"
		
	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"
	
	CommandLen = 4

	HeadLen = 1
	
	// 命令头数据
	HeadData = [0x80]
	
	// 地址码
	RS485Addr = 0xFF
	
	// 是否有弹起消息
	UpMsg = 1
	
	// 是否通过脚本解析
	ParseMode = 1
	
	// 命令码
	KEYOperator = {
				KEYZERO = [0, KEYVALUE.KEY_0],
				KEYONE = [1, KEYVALUE.KEY_1],
				KEYTWO = [2, KEYVALUE.KEY_2],
				KEYTHREE = [3, KEYVALUE.KEY_3],
				KEYFOUR = [4, KEYVALUE.KEY_4], 
				KEYFIVE = [5, KEYVALUE.KEY_5],
				KEYSIX = [6, KEYVALUE.KEY_6],
				KEYSEVEN = [7, KEYVALUE.KEY_7],
				KEYEIGHT = [8, KEYVALUE.KEY_8],
				KEYNINE = [9, KEYVALUE.KEY_9],
				KEYUP = [10, KEYVALUE.KEY_UP],
				KEYDOWN = [11, KEYVALUE.KEY_DOWN],
				KEYLEFT = [12, KEYVALUE.KEY_LEFT],
				KEYRIGHT = [13, KEYVALUE.KEY_RIGHT],
				KEYMUTE = [14, KEYVALUE.KEY_ESC],
				KEYMENU = [15, KEYVALUE.KEY_RET],
				KEYREC = [16, KEYVALUE.KEY_REC],
				KEYDISP = [17, KEYVALUE.KEY_SPLIT],
				KEYPALY = [18, KEYVALUE.KEY_PLAY],
				KEYPTZ = [19, KEYVALUE.KEY_PTZ],
				KEYPREV = [20, KEYVALUE.KEY_PREV],
				KEYNEXT = [21, KEYVALUE.KEY_NEXT],
				KEYSLOW = [22, KEYVALUE.KEY_SLOW],
				KEYFAST = [23, KEYVALUE.KEY_FAST],
				KEYINFO = [24, KEYVALUE.KEY_INFO],
				KEYWIN1_4 = [25, KEYVALUE.KEY_SPLIT4],
				KEYWIN2_4 = [26, KEYVALUE.KEY_SPLIT5_8],
				KEYWIN3_4 = [27, KEYVALUE.KEY_SPLIT9_12],
				KEYWIN4_4 = [28, KEYVALUE.KEY_SPLIT13_16],
				KEYWIN1_8 = [29, KEYVALUE.KEY_SPLIT9],
				KEYWIN2_8 = [30, KEYVALUE.KEY_SPLIT9],
				KEYWIN1_16 = [31, KEYVALUE.KEY_SPLIT16],
				KEYIRISOPEN = [32, KEYVALUE.KEY_IRIS_OPEN],
				KEYIRISCLOSE = [33, KEYVALUE.KEY_IRIS_CLOSE],
				KEYFOCUSFAR = [34, KEYVALUE.KEY_FOCUS_FAR],
				KEYFOCUSNEAR = [35, KEYVALUE.KEY_FOCUS_NEAR],
				KEYTELE = [36, KEYVALUE.KEY_TELE],
				KEYWIDE = [37, KEYVALUE.KEY_WIDE],
				KEYPTZUP = [42, KEYVALUE.KEY_UP],
				KEYPTZDOWN = [43, KEYVALUE.KEY_DOWN],
				KEYPTZLEFT = [44, KEYVALUE.KEY_LEFT],
				KEYPTZRIGHT = [45, KEYVALUE.KEY_RIGHT],
				KEYLOCK = [64, KEYVALUE.KEY_UNKNOWN],
				KEYEXCHANGE = [65, KEYVALUE.KEY_UNKNOWN]
				}
				
	function setAddr(addr)
	{
		RS485Addr = addr;
	}
	
	function CheckData(cmdBuf)
	{
		//::print("CheckData\n");
		local addr = cmdBuf[1];
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
				if (tmp[0] == cmdBuf[2])
				{
					if (0 == cmdBuf[3])
					{
						ret = tmp[1] | 0x1000;
						return ret;
					}
					else
					{
						return tmp[1];
					}
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

local cLongYang = LongYang();

return cLongYang;