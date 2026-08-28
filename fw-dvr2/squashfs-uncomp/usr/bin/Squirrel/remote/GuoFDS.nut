class GuoFDS extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "GuoFDS"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEYPOW = [0x00,0xff],//KeyPower(null)
				KEYREC = [0x01,0x93],//KEY_REC
				KEYONE = [0x11,0x87],//Key1
				KEYTWO = [0x12,0x86],//Key2
				KEYTHREE = [0x13,0x85],//Key3
				KEYFOUR = [0x14,0x8b],//Key4
				KEYFIVE = [0x15,0x8a],//Key5
				KEYSIX = [0x16,0x89],//Key6
				KEYSEVEN = [0x17,0x8f],//Key7
				KEYEIGHT = [0x18,0x8e],//Key8
				KEYNINE = [0x19,0x8d],//Key9
				KEYZERO = [0x10,0x92],//Key0
				KEYPLUS = [0x1B,0x80],//10+
				KEYBACK = [0x1A,0x98],//KeyBACK
				KEYMENU = [0x02,0xa5],//KeyMenu
				KEYMULT = [0x03,0x84],//KeyMult
				KEYUP = [0x05,0x95],//KeyUp
				KEYLEFT = [0x07,0x9b],//KeyLeft
				KEYRIGHT = [0x08,0x99],//KeyRight
				KEYDOWN = [0x06,0x9a],//KeyDown
				KEYENTER = [0x04,0x9e],//KeyEnter
				KEYFN = [0x09,0x91],//KeyFN
				KEYESC = [0x0A,0x96],//KeyESC
				KEYFAST = [0x20,0x9c],//KeyFast
				KEYSLOW = [0x21,0x8c],//KeySlow
				KEYPREV = [0x22,0x90],//KeyPrev
				KEYNEXT = [0x23,0x83],//KeyNext
				KEYSEARCH = [0x24,0x88],//KeySearch
				KEYPLAY = [0x25,0x82],//KeyPlay
				KEYSTOP = [0x26,0x81],//KeyStop
				KEYMUTE = [0x27,0xa3],//KeyMute
				KEYPTZ = [0x28,0xa4],//KeyPtz
				KEYINVALIDONE = [0x29,0xff],//KeyAlarm(null)
				KEYINVALIDTWO = [0x2A,0xff],//KeyClear(null)
				KEYADDR = [0x2B,0x97],//KeyAddr
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

local cGuoFDS = GuoFDS();

return cGuoFDS;