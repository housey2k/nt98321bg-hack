class Vista extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "Vista"  //本协议与Hikvision协议是完全一样的，只是名字不一样
		
	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"
	
	CommandLen = 6

	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xE0]
	
	// 地址码
	RS485Addr = 0xFF
	
	// 是否有弹起消息
	UpMsg = 1
	
	// 是否通过脚本解析
	ParseMode = 1
	
	// 命令码
	KEYOperator = {
				KEYDISP = [0x28, KEYVALUE.KEY_ESC],
				KEYREC = [0xd8, KEYVALUE.KEY_REC],
				KEYPALY = [0x58, KEYVALUE.KEY_PLAY],
				KEYONE = [0x80, KEYVALUE.KEY_1],
				KEYTWO = [0x40, KEYVALUE.KEY_2],
				KEYTHREE = [0xc0, KEYVALUE.KEY_3],
				KEYFOUR = [0x20, KEYVALUE.KEY_4], 
				KEYFIVE = [0xa0, KEYVALUE.KEY_5],
				KEYSIX = [0x60, KEYVALUE.KEY_6],
				KEYSEVEN = [0xe0, KEYVALUE.KEY_7],
				KEYEIGHT = [0x10, KEYVALUE.KEY_8],
				KEYNINE = [0x90, KEYVALUE.KEY_9],
				KEYZERO = [0x00, KEYVALUE.KEY_0],
				KEYENTER = [0xf0, KEYVALUE.KEY_RET],
				KEYSPLIT = [0x81, KEYVALUE.KEY_SPLIT],
				KEYSPLIT2 = [0xb8, KEYVALUE.KEY_SPLIT],
				KEYFUNC = [0x61, KEYVALUE.KEY_FUNC],
				KEYPTZ = [0x21, KEYVALUE.KEY_PTZ],
				KEYPTZ2 = [0x38, KEYVALUE.KEY_PTZ],
				KEYMENU = [0x82, KEYVALUE.KEY_MENU],
				KEYSHUT = [0x44, KEYVALUE.KEY_SHUT],
				KEYUP = [0xb0, KEYVALUE.KEY_UP],
				KEYDOWN = [0x70, KEYVALUE.KEY_DOWN],
				KEYLEFT = [0x50, KEYVALUE.KEY_LEFT],
				KEYRIGHT = [0xd0, KEYVALUE.KEY_RIGHT],
				}	
	
	function setAddr(addr)
	{
		RS485Addr = addr;
		HeadData[0] = ((0xE0 + RS485Addr - 1) & 0xFF);
	}
	
	function CheckData(cmdBuf)
	{
		//::print("CheckData\n");
		local sum1 = 0;
		local sum2 = 0;
		local i = 0;
		local addr = 0;
		
		for (i = 0; i < CommandLen - 1; i++)
		{
			sum1 = sum1 ^ cmdBuf[i];
		}
		sum2 = cmdBuf[CommandLen - 1];
		if (sum1 != sum2)
		{
			::print("sum1: "+sum1+" sum2: "+sum2+"\n");
			return -1;
		}
		addr = cmdBuf[0];
		if (addr != HeadData[0])
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
			if(cmdBuf[2] == 0xa0)
			{
				foreach(k,v in KEYOperator)
				{
					local tmp = KEYOperator.rawget(k);
					if (tmp[0] == cmdBuf[3])
					{
						if (0 == cmdBuf[4])
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
		else
		{
			return KEYVALUE.KEY_UNKNOWN;
		}
	}
}

local CVista = Vista();

return CVista;