class DaHua2 extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "DaHua2"

	// 指明是云台协议还是矩阵协议，使用"RS485"表示
	Type = "RS485"

	CommandLen = 8

	HeadLen = 1

	// 命令头数据
	HeadData = [0x90]

	// 地址码
	RS485Addr = 0x00

	// 有弹起消息
	UpMsg = 1

	// 通过脚本解析
	ParseMode = 1

	// 命令码
	KEYOperator = {
				KEYZERO = [0x00, KEYVALUE.KEY_0],
				KEYONE = [0x01, KEYVALUE.KEY_1],
				KEYTWO = [0x02, KEYVALUE.KEY_2],
				KEYTHREE = [0x03, KEYVALUE.KEY_3],
				KEYFOUR = [0x04, KEYVALUE.KEY_4], 
				KEYFIVE = [0x05, KEYVALUE.KEY_5],
				KEYSIX = [0x06, KEYVALUE.KEY_6],
				KEYSEVEN = [0x07, KEYVALUE.KEY_7],
				KEYEIGHT = [0x08, KEYVALUE.KEY_8],
				KEYNINE = [0x09, KEYVALUE.KEY_9],

				KEYUP0 = [0x0a, KEYVALUE.KEY_UP],
				KEYDOWN0 = [0x0b, KEYVALUE.KEY_DOWN],
				KEYLEFT0 = [0x0c, KEYVALUE.KEY_LEFT],
				KEYRIGHT0 = [0x0d, KEYVALUE.KEY_RIGHT],
				
				KEYESC0 = [0x0e, KEYVALUE.KEY_ESC],
				KEYENTER = [0x0f, KEYVALUE.KEY_RET],

				//Handler key
				KEYUP1 = [0x41, KEYVALUE.KEY_UP],
				KEYDOWN1 = [0x42, KEYVALUE.KEY_DOWN],
				KEYLEFT1 = [0x43, KEYVALUE.KEY_LEFT],
				KEYRIGHT1 = [0x44, KEYVALUE.KEY_RIGHT],
				KEYLEFT2 = [0x45, KEYVALUE.KEY_LEFT],   //left up    -> left
				KEYLEFT3 = [0x46, KEYVALUE.KEY_LEFT],   //left down  -> left
				KEYRIGHT2 = [0x47, KEYVALUE.KEY_RIGHT], //right up   -> right
				KEYRIGHT3 = [0x48, KEYVALUE.KEY_RIGHT], //right down -> right
				KEYWIDE0 = [0x49, KEYVALUE.KEY_WIDE],
				KEYTELE0 = [0x4a, KEYVALUE.KEY_TELE],

				KEYPALY = [0x10, KEYVALUE.KEY_PLAY],
				KEYESC1 = [0x11, KEYVALUE.KEY_ESC],
				KEYFAST = [0x12, KEYVALUE.KEY_FAST],
				KEYSLOW = [0x13, KEYVALUE.KEY_SLOW],
				KEYPREV = [0x14, KEYVALUE.KEY_PREV],
				KEYNEXT = [0x15, KEYVALUE.KEY_NEXT],
				KEYBACK = [0x16, KEYVALUE.KEY_BACK],
				KEYNEXTFRAME = [0x17, KEYVALUE.KEY_NEXTFRAME],
				KEYFASTBACK = [0x18, KEYVALUE.KEY_FASTBACK],
				KEYREC = [0x19, KEYVALUE.KEY_REC],
				KEYSPLIT = [0x1a, KEYVALUE.KEY_SPLIT],
				KEYSPLIT1 = [0x1b, KEYVALUE.KEY_SPLIT1],
				KEYSPLIT4  = [0x1c, KEYVALUE.KEY_SPLIT4],
				KEYSPLIT9  = [0x1d, KEYVALUE.KEY_SPLIT9],
				KEYSPLIT16 = [0x1e, KEYVALUE.KEY_SPLIT16],

				KEYFUNC0 = [0x1f, KEYVALUE.KEY_FUNC],
				KEYFUNC1 = [0x20, KEYVALUE.KEY_FUNC],

				KEYPTZ = [0x21, KEYVALUE.KEY_PTZ],
				KEYWIDE1 = [0x22, KEYVALUE.KEY_WIDE],
				KEYTELE1 = [0x23, KEYVALUE.KEY_TELE],
				KEYIRISCLOSE = [0x24, KEYVALUE.KEY_IRIS_CLOSE],
				KEYIRISOPEN = [0x25, KEYVALUE.KEY_IRIS_OPEN],
				KEYRFOCUSNEAR = [0x26, KEYVALUE.KEY_FOCUS_NEAR],
				KEYFOCUSFAR = [0x27, KEYVALUE.KEY_FOCUS_FAR],
				KEYBRUSH = [0x28, KEYVALUE.KEY_BRUSH],
				KEYLIGHT = [0x29, KEYVALUE.KEY_LIGHT],
				KEYSCAN = [0x2a, KEYVALUE.KEY_SCANON],
				KEYTOUR = [0x2b, KEYVALUE.KEY_AUTOTOUR],
				KEYPATTERN = [0x2c, KEYVALUE.KEY_PATTERN],
				KEYSETPRE = [0x2d, KEYVALUE.KEY_SPRESET],
				KEYGOTOPRE = [0x2e, KEYVALUE.KEY_GPRESET],
				KEYDELPRE = [0x2f, KEYVALUE.KEY_DPRESET],

				KEYALARM = [0x3f, KEYVALUE.KEY_ALARM],
				}	

	function setAddr(addr)
	{
		RS485Addr = addr & 0xFF;
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
			sum1 = sum1 + cmdBuf[i];
		}
		sum2 = cmdBuf[CommandLen - 1];
		if (sum1 != sum2)
		{
			::print("sum1: "+sum1+" sum2: "+sum2+"\n");
			return -1;
		}
		addr = cmdBuf[1];
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

local CDaHua2 = DaHua2();

return CDaHua2;