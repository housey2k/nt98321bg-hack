class TaiA extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "TaiA"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEYPOW = [0x0F,0xff],//KEY_POWER
				KEYADDR = [0x12,0x97],//KEY_ADDR
				KEYONE = [0x01,0x87],//Key1
				KEYTWO = [0x02,0x86],//Key2
				KEYTHREE = [0x03,0x85],//Key3
				KEYFOUR = [0x04,0x8b],//Key4
				KEYFIVE = [0x05,0x8a],//Key5
				KEYSIX = [0x06,0x89],//Key6
				KEYSEVEN = [0x07,0x8f],//Key7
				KEYEIGHT = [0x08,0x8e],//Key8
				KEYNINE = [0x09,0x8d],//Key9
				KEYZERO = [0x0a,0x92],//Key0
				KEYFUNC = [0x13,0x91],//KEY_FUNC
				KEYESC1 = [0x1c,0x96],//KEY_ESC
				KEYMENU = [0x15,0xa5],//KeyMenu
				KEYESC2 = [0x17,0x96],//KeyESC
				KEYUP = [0x1b,0x95],//KeyUp
				KEYLEFT = [0x1e,0x9b],//KeyLeft
				KEYRIGHT = [0x1a,0x99],//KeyRight
				KEYDOWN = [0x1f,0x9a],//KeyDown
				KEYENTER = [0x10,0x9e],//KeyEnter
				KEYADD = [0x0c,0xa0],//KeyAdd
				KEYDEC = [0x18,0xa1],//KeyDec
				KEYREC = [0x19,0x93],//KeyRec
				KEYSEARCH = [0x0e,0x88],//KeySearch
				KEYPLAY = [0x1d,0x82],//KeyPlay
				KEYSLOW = [0x14,0x8c],//KeySlow
				KEYFAST = [0x40,0x9c],//KeyFast
				KEYSTOP = [0x42,0x81],//KeyStop
				KEYPTZ = [0x0b,0xa4],//KeyPtz
				KEYMUTE = [0x41,0xa3],//KeyMute
				KEYBACK = [0x43,0x98],//KEY_BACK
				KEYALARM = [0x11,0xa2],//KEY_ALARM
				KEYSPLIT = [0x44,0x84],//KEY_SPLIT
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

local cTaiA = TaiA();

return cTaiA;