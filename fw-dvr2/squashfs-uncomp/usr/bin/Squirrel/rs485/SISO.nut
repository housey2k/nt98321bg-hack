class SISO extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "SISO"
		
	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"
	
	CommandLen = 8

	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xF3]
	
	// 地址码
	RS485Addr = 0xFF
	
	// 是否有弹起消息
	UpMsg = 1
	
	// 是否通过脚本解析
	ParseMode = 1
	
	KEYOperator = {
				KEYSELDVR = [0x31, 0x00, 0x00, KEYVALUE.KEY_UNKNOWN],
				KEYSPLIT 		= [0x33, 0x01, 0x00, KEYVALUE.KEY_SPLIT],
				KEYSPLIT1 	= [0x34, 0x01, 0x00, KEYVALUE.KEY_SPLIT1],
				KEYSPLIT4 	= [0x34, 0x04, 0x00, KEYVALUE.KEY_SPLIT4],
				KEYSPLIT9 	= [0x34, 0x09, 0x00, KEYVALUE.KEY_SPLIT9],
				KEYSPLIT16	= [0x34, 0x10, 0x00, KEYVALUE.KEY_SPLIT16],
				KEYREC			= [0x35, 0x02, 0x00, KEYVALUE.KEY_REC],
				KEYPLAY			= [0x35, 0x04, 0x00, KEYVALUE.KEY_PLAY],
				KEYPAUSE		= [0x36, 0x01, 0x00, KEYVALUE.KEY_PAUSE],
				KEYSTOP			= [0x36, 0x02, 0x00, KEYVALUE.KEY_STOP],
				KEYSLOW			= [0x37, 0x01, 0x00, KEYVALUE.KEY_SLOW],
				KEYFAST			= [0x37, 0x02, 0x00, KEYVALUE.KEY_FAST],
				KEYPREV			= [0x38, 0x01, 0x00, KEYVALUE.KEY_PREV],
				KEYNEXT			= [0x38, 0x02, 0x00, KEYVALUE.KEY_NEXT],
				KEYPREVFRAME= [0x39, 0x01, 0x00, KEYVALUE.KEY_PREVFRAME],
				KEYNEXTFRAME= [0x39, 0x02, 0x00, KEYVALUE.KEY_NEXTFRAME],
				KEYSNAP			= [0x3A, 0x01, 0x00, KEYVALUE.KEY_SNAP],
				KEYTIP			= [0x3B, 0x00, 0x00, KEYVALUE.KEY_UNKNOWN],
				KEY0				= [0x3E, 0x00, 0x00, KEYVALUE.KEY_0],
				KEY1				= [0x3E, 0x01, 0x00, KEYVALUE.KEY_1],
				KEY2				= [0x3E, 0x02, 0x00, KEYVALUE.KEY_2],
				KEY3				= [0x3E, 0x03, 0x00, KEYVALUE.KEY_3],
				KEY4				= [0x3E, 0x04, 0x00, KEYVALUE.KEY_4],
				KEY5				= [0x3E, 0x05, 0x00, KEYVALUE.KEY_5],
				KEY6				= [0x3E, 0x06, 0x00, KEYVALUE.KEY_6],
				KEY7				= [0x3E, 0x07, 0x00, KEYVALUE.KEY_7],
				KEY8				= [0x3E, 0x08, 0x00, KEYVALUE.KEY_8],
				KEY9				= [0x3E, 0x09, 0x00, KEYVALUE.KEY_9],
				KEYUP				= [0x3E, 0x0A, 0x00, KEYVALUE.KEY_UP],
				KEYDOWN			= [0x3E, 0x0B, 0x00, KEYVALUE.KEY_DOWN],
				KEYLEFT			= [0x3E, 0x0C, 0x00, KEYVALUE.KEY_LEFT],
				KEYRIGHT		= [0x3E, 0x0D, 0x00, KEYVALUE.KEY_RIGHT],
				KEYRET			= [0x3E, 0x0E, 0x00, KEYVALUE.KEY_RET],
				KEYESC			= [0x3E, 0x0F, 0x00, KEYVALUE.KEY_ESC],
				KEYMENU			= [0x3E, 0x15, 0x00, KEYVALUE.KEY_RET],
				KEYEXIT			= [0x3E, 0x16, 0x00, KEYVALUE.KEY_ESC],
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
			sum1 = sum1 ^ cmdBuf[i];
		}
		sum2 = cmdBuf[CommandLen - 1];
		if (sum1 != sum2)
		{
			::print("sum1: "+sum1+" sum2: "+sum2+"\n");
			return -1;
		}
		addr = cmdBuf[1] << 8 | cmdBuf[2];
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
		local key = KEYVALUE.KEY_UNKNOWN;

		ret = CheckData(cmdBuf);
		if (ret == 0)
		{
			foreach(k,v in KEYOperator)
			{
				local tmp = KEYOperator.rawget(k);
				if (cmdBuf[3] == tmp[0] && cmdBuf[4] == tmp[1] && cmdBuf[5] == tmp[2])
				{
					return tmp[3];
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

local cSISO = SISO();

return cSISO;