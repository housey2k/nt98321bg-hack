class LongYangD900 extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "LongYang-D900"
		
	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"
	
	CommandLen = 8

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
				KEYMENU = [0x41, 0x00, KEYVALUE.KEY_MENU],
				KEYALARM = [0x21, 0x00, KEYVALUE.KEY_ALARM],
				KEYENTER = [0x10, 0x00, KEYVALUE.KEY_RET],
				KEYREC = [0x42, 0x00, KEYVALUE.KEY_REC],
				KEYSTOP = [0x43, 0x00, KEYVALUE.KEY_STOP],
				KEYPLAY = [0x44, 0x00, KEYVALUE.KEY_PLAY],
				KEYSEARCH = [0x4A, 0x00, KEYVALUE.KEY_SEARCH],
				KEYSLOW = [0x4E, 0x00, KEYVALUE.KEY_SLOW],
				KEYPAUSE = [0x4F, 0x00, KEYVALUE.KEY_PAUSE],
				KEYBACK = [0x4C, 0x00, KEYVALUE.KEY_BACK],
				KEYFAST = [0x4D, 0x00, KEYVALUE.KEY_FAST],
				KEYZERO = [0x11, 0x00, KEYVALUE.KEY_0],
				KEYONE = [0x11, 0x01, KEYVALUE.KEY_1],
				KEYTWO = [0x11, 0x02, KEYVALUE.KEY_2],
				KEYTHREE = [0x11, 0x03, KEYVALUE.KEY_3],
				KEYFOUR = [0x11, 0x04, KEYVALUE.KEY_4], 
				KEYFIVE = [0x11, 0x05, KEYVALUE.KEY_5],
				KEYSIX = [0x11, 0x06, KEYVALUE.KEY_6],
				KEYSEVEN = [0x11, 0x07, KEYVALUE.KEY_7],
				KEYEIGHT = [0x11, 0x08, KEYVALUE.KEY_8],
				KEYNINE = [0x11, 0x09, KEYVALUE.KEY_9],
				KEYSPLIT = [0x46, 0x00, KEYVALUE.KEY_SPLIT],
				KEYSPLIT4 = [0x48, 0x00, KEYVALUE.KEY_SPLIT4],
				KEYSPLIT9 = [0x47, 0x00, KEYVALUE.KEY_SPLIT9],
				KEYESC = [0x12, 0x00, KEYVALUE.KEY_ESC],
				KEYPTZ = [0x50, 0x00, KEYVALUE.KEY_PTZ],
				KEYTELE = [0x4B, 0x00, KEYVALUE.KEY_TELE],
				KEYWIDE = [0x5F, 0x00, KEYVALUE.KEY_WIDE],
				KEYUP = [0x54, 0x00, KEYVALUE.KEY_UP],
				KEYDOWN = [0x55, 0x00, KEYVALUE.KEY_DOWN],
				KEYLEFT = [0x56, 0x00, KEYVALUE.KEY_LEFT],
				KEYRIGHT = [0x57, 0x00, KEYVALUE.KEY_RIGHT]
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
				if(tmp[0] == cmdBuf[2] && tmp[1] == cmdBuf[3])
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

local cLongYangD900 = LongYangD900();

return cLongYangD900;