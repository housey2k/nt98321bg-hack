class MinYang extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "MinYang"
		
	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"
	
	CommandLen = 11

	HeadLen = 2
	
	// 命令头数据
	HeadData = [0xAA, 0x75]
	
	// 地址码
	RS485Addr = 0xFF
	
	// 是否有弹起消息
	UpMsg = 0
	
	// 是否通过脚本解析
	ParseMode = 1
	
	// 命令码
	PTZOperator = {
				PTZSTOP = [0, KEYVALUE.KEY_UNKNOWN],
				PTZLEFT = [1, KEYVALUE.KEY_LEFT],
				PTZRIGHT = [2, KEYVALUE.KEY_RIGHT],
				PTZUP = [3, KEYVALUE.KEY_UP], 
				PTZDOWN = [4, KEYVALUE.KEY_DOWN],
				PTZIRISCLOSE = [5, KEYVALUE.KEY_IRIS_CLOSE],
				PTZIRISOPEN = [6, KEYVALUE.KEY_IRIS_OPEN],
				PTZTELE = [7, KEYVALUE.KEY_TELE],
				PTZWIDE = [8, KEYVALUE.KEY_WIDE],
				PTZFOCUSNEAR = [9, KEYVALUE.KEY_FOCUS_NEAR],
				PTZFOCUSFAR = [10, KEYVALUE.KEY_FOCUS_FAR]
				}
	
	KEYOperator = {
				KEYONE = [2, KEYVALUE.KEY_1],
				KEYTWO = [3, KEYVALUE.KEY_2],
				KEYTHREE = [4, KEYVALUE.KEY_3],
				KEYFOUR = [5, KEYVALUE.KEY_4], 
				KEYFIVE = [6, KEYVALUE.KEY_5],
				KEYSIX = [7, KEYVALUE.KEY_6],
				KEYSEVEN = [8, KEYVALUE.KEY_7],
				KEYEIGHT = [9, KEYVALUE.KEY_8],
				KEYNINE = [10, KEYVALUE.KEY_9],
				KEYTEN = [11, KEYVALUE.KEY_10],
				KEYDISP = [29, KEYVALUE.KEY_SPLIT],
				KEYMUTE = [47, KEYVALUE.KEY_ESC],
				KEYREC = [58, KEYVALUE.KEY_REC],
				KEYFRAME = [35, KEYVALUE.KEY_STEPX], 
				KEYPALY = [34, KEYVALUE.KEY_PLAY],
				KEYSTOP = [36, KEYVALUE.KEY_STOP],
				KEYMENU = [39, KEYVALUE.KEY_MENU],
				KEYLEFT = [20, KEYVALUE.KEY_LEFT],
				KEYRIGHT = [21, KEYVALUE.KEY_RIGHT],
				KEYPTZ = [22, KEYVALUE.KEY_PTZ],
				KEYUP = [18, KEYVALUE.KEY_UP],
				KEYDOWN = [19, KEYVALUE.KEY_DOWN]
				}
				
	function setAddr(addr)
	{
		RS485Addr = addr;
	}
	
	function CheckData(cmdBuf)
	{
		//::print("CheckData\n");
		local sum1 = 0;
		local sum2 = 0;
		local i = 0;
		local addr = 0;
		
		for (i = 0; i < CommandLen - 2; i++)
		{
			sum1 += cmdBuf[i];
		}
		sum2 = cmdBuf[CommandLen - 2] << 8 | cmdBuf[CommandLen - 1];
		if (sum1 != sum2)
		{
			::print("sum1: "+sum1+" sum2: "+sum2+"\n");
			return -1;
		}
		addr = (cmdBuf[5] ) | (cmdBuf[4] << 8) | (cmdBuf[3] << 16) | (cmdBuf[2] << 24);
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
			// 云台
			if (cmdBuf[6] == 0x55)
			{	
				foreach(k,v in PTZOperator)
				{
					local tmp = PTZOperator.rawget(k);
					if (tmp[0] == cmdBuf[8])
					{
						return tmp[1];
					}
				}
				return KEYVALUE.KEY_UNKNOWN;
			}
			else if(cmdBuf[6] == 0xaa)
			{
				foreach(k,v in KEYOperator)
				{
					local tmp = KEYOperator.rawget(k);
					if (tmp[0] == cmdBuf[8])
					{
						return tmp[1];
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

local cMinYang = MinYang();

return cMinYang;