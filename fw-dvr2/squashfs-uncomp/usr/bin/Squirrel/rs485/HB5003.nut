class HB5003 extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "HB5003"
		
	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"
	
	CommandLen = 7

	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xF6]
	
	// 地址码
	RS485Addr = 0xFF
	
	// 是否有弹起消息
	UpMsg = 1
	
	// 是否通过脚本解析
	ParseMode = 1

	LastCmd = 0x00
	
	// 命令码
	KEYOperator = {
				KEYZERO  = [0xEF, KEYVALUE.KEY_0],
				KEYONE   = [0x8D, KEYVALUE.KEY_1],
				KEYTWO   = [0xAD, KEYVALUE.KEY_2],
				KEYTHREE = [0x9D, KEYVALUE.KEY_3],
				KEYFOUR  = [0x4D, KEYVALUE.KEY_4], 
				KEYFIVE  = [0x6D, KEYVALUE.KEY_5],
				KEYSIX   = [0x5D, KEYVALUE.KEY_6],
				KEYSEVEN = [0xCD, KEYVALUE.KEY_7],
				KEYEIGHT = [0xED, KEYVALUE.KEY_8],
				KEYNINE  = [0xDD, KEYVALUE.KEY_9],
				KEYUP    = [0x1F, KEYVALUE.KEY_UP],
				KEYDOWN  = [0x2F, KEYVALUE.KEY_DOWN],
				KEYLEFT  = [0x8F, KEYVALUE.KEY_LEFT],
				KEYRIGHT = [0xAF, KEYVALUE.KEY_RIGHT],
				KEYENTER = [0x37, KEYVALUE.KEY_RET],
				KEYESC   = [0x07, KEYVALUE.KEY_ESC],
				KEYREC   = [0x0D, KEYVALUE.KEY_REC],
				KEYPALY  = [0x2D, KEYVALUE.KEY_PLAY],
				KEYMENU  = [0x1D, KEYVALUE.KEY_MENU],
				KEYBACK  = [0x3D, KEYVALUE.KEY_BACKUP],
				KEYSPLIT = [0xBD, KEYVALUE.KEY_SPLIT],
				KEYALARM = [0x73, KEYVALUE.KEY_ALARM],
				KEYPGUP  = [0x0F, KEYVALUE.KEY_PGUP],
				KEYPGDN  = [0x9F, KEYVALUE.KEY_PGDN],
				KEYNEXTFRAME = [0x15, KEYVALUE.KEY_NEXTFRAME],
				KEYFAST  = [0xB5, KEYVALUE.KEY_FAST],
				KEYSLOW  = [0x35, KEYVALUE.KEY_SLOW],
				KEYSEARCH = [0x25, KEYVALUE.KEY_SEARCH],
				KEYESC   = [0xDF,KEYVALUE.KEY_ESC],
				}	
	
	function setAddr(addr)
	{
		RS485Addr = addr;
	}
	
	function CheckData(cmdBuf)
	{
		local sum1 = 0;
		local sum2 = 0;
		local i = 0;
		local addr = 0;
		
		for (i = 1; i < CommandLen - 1; i++)
		{
			sum1 = sum1 + cmdBuf[i];
		}
		sum1 = sum1 & 0x7f;
		sum2 = cmdBuf[CommandLen - 1];
		if (sum1 != sum2)
		{
			::print("sum1: "+sum1+" sum2: "+sum2+"\n");
			return -1;
		}
		addr = cmdBuf[2];
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

				if (tmp[0] == cmdBuf[4])
				{
					LastCmd = tmp[1];
					return tmp[1];
				}
				else if (cmdBuf[4] == 0)
				{
					ret = LastCmd | 0x1000;
					return ret;
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

local CHB5003 = HB5003();

return CHB5003;