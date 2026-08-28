class DeJL extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "DeJL"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEYPOW = [0x45,0xff],//KEY_POWER
				KEYADDR = [0x41,0x97],//KEY_ADDR
				KEYONE = [0x09,0x87],//Key1
				KEYTWO = [0x1d,0x86],//Key2
				KEYTHREE = [0x1f,0x85],//Key3
				KEYFOUR = [0x0d,0x8b],//Key4
				KEYFIVE = [0x19,0x8a],//Key5
				KEYSIX = [0x1b,0x89],//Key6
				KEYSEVEN = [0x11,0x8f],//Key7
				KEYEIGHT = [0x15,0x8e],//Key8
				KEYNINE = [0x17,0x8d],//Key9
				KEYZERO = [0x12,0x92],//Key0
				KEYFN = [0x57,0x91],//KeyFN
				KEYBACK = [0x52,0x98],//KeyBACK
				KEYMENU = [0x51,0xa5],//KeyMenu
				KEYESC = [0x5c,0x96],//KeyESC
				KEYUP = [0x03,0x95],//KeyUp
				KEYLEFT = [0x0e,0x9b],//KeyLeft
				KEYRIGHT = [0x1a,0x99],//KeyRight
				KEYDOWN = [0x02,0x9a],//KeyDown
				KEYDOWN = [0x07,0x9e],//KeyEnter
				KEYFOCUSADD = [0x14,0xa6],//KeyFOCUS+
				KEYFOCUSREDUCE = [0x1C,0xa7],//KeyFOCUS_
				KEYREC = [0x4e,0x93],//KeyRec
				KEYSEARCH = [0x4a,0x88],//KeySearch
				KEYPLAY = [0x53,0x82],//KeyPlay
				KEYSLOW = [0x08,0x8c],//KeySlow
				KEYFAST = [0x10,0x9c],//KeyFast
				KEYSTOP = [0x4f,0x81],//KeyStop
				KEYPTZ = [0x49,0xa4],//KeyPtz
				KEYMUTE = [0x0c,0xa3],//KeyMute
				KEYDIGIT = [0x56,0x94],//双数
				KEYPREV = [0x13,0x90],//PREV
				KEYNEXT = [0x06,0x83],//NEXT
				KEYSPLIT = [0x50,0x84],//KEY_SPLIT
				KEYZOOMADD = [0X48,0xa0],//ZOOM+/
				KEYZOOMREDUCE = [0X18,0xa1],//ZOOM-/
				KEYPREVFRAME = [0x00,0xb0],//PREV_FRAME/
				KEYNEXTFRAME = [0x0a,0xb1],//NEXT_FRAME/
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

local cDeJL = DeJL();

return cDeJL;