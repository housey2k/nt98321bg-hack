class HaiRSX extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "HaiRSX"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEYPOW = [0x0a,0xa8],//KEY_POWER
				KEYADDR = [0x19,0x97],//KEY_ADDR
				KEYONE = [0x01,0x87],//Key1
				KEYTWO = [0x02,0x86],//Key2
				KEYTHREE = [0x03,0x85],//Key3
				KEYFOUR = [0x04,0x8b],//Key4
				KEYFIVE = [0x05,0x8a],//Key5
				KEYSIX = [0x06,0x89],//Key6
				KEYSEVEN = [0x07,0x8f],//Key7
				KEYEIGHT = [0x08,0x8e],//Key8
				KEYNINE = [0x09,0x8d],//Key9
				KEYPLUS = [0x40,0x80],//Key10+
				KEYZERO = [0x00,0x92],//Key0
				KEYSPLIT = [0x45,0x84],//KEY_SPLIT
				KEYSPLITFOUR = [0x16,0xac],//Key_SPLIT4
				KEYSPLITEIGHT = [0x1f,0xad],//KEY_SPLIT8
				KEYUP = [0x0b,0x95],//KeyUP
				KEYDOWN = [0x0e,0x9a],//KeyDown
				KEYLEFT = [0x11,0x9b],//KeyLeft
				KEYRIGHT = [0x10,0x99],//KeyRight
				KEYENTER = [0x0d,0x9e],//KeyEnter
				KEYMENU = [0x1a,0xa5],//KeyMenu
				KEYSEQ = [0x1b,0xaf],//KeySEQ
				KEYPLAY = [0x1c,0x82],//KeyPlay
				KEYPREVFRAME = [0x41,0xb0],//KeyPreFrame
				KEYNEXTFRAME = [0x42,0xb1],//KeyNextFrame
				KEYPREV = [0x15,0x90],//KeyPre
				KEYNEXT = [0x18,0x83],//KeyNext
				KEYRECORD = [0x44,0x93],//KeyRecord
				KEYPTZ = [0x0c,0xa4],//KeyPTZ
				KEYZOOMADD = [0x14,0xa0],//KeyZoom+
				KEYZOOMREDUCE = [0x17,0xa1],//KeyZoom-
				KEYFOCUSADD = [0x1d,0xa6],//KeyFocus+
				KEYFOCUSREDUCE = [0x43,0xa7],//KeyFocus+
			}
			
	function ParseData(cmdBuf)
	{
		foreach(k,v in PadCodeFunc)
		{
			local tmp = PadCodeFunc.rawget(k);
			if (cmdBuf[2] == tmp[0])
			{
				local buf = [];
				buf.insert(0, cmdBuf[0]);
				buf.insert(1, cmdBuf[1]);
				buf.insert(2, tmp[1]);
				buf.insert(3, cmdBuf[3]);
				return buf;
			}
		}
	}
}

local cHaiRSX = HaiRSX();

return cHaiRSX;