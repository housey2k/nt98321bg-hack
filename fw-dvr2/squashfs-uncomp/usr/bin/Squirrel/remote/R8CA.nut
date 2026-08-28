class R8CA extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "R8CA"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEYSPLIT  = [0x0d,0x84],//KEY_SPLIT
				KEYSEARCH = [0x0c,0x88],//KEY_SEARCH
				KEYONE = [0x01,0x87],//Key1
				KEYTWO = [0x02,0x86],//Key2
				KEYTHREE = [0x03,0x85],//Key3
				KEYFOUR = [0x04,0x8b],//Key4
				KEYFIVE = [0x05,0x8a],//Key5
				KEYSIX = [0x06,0x89],//Key6
				KEYSEVEN = [0x07,0x8f],//Key7
				KEYEIGHT = [0x08,0x8e],//Key8
				KEYNINE = [0x09,0x8d],//Key9
				KEYZERO = [0x00,0x92],//Key0
				KEYFUNC = [0x0e,0x91],//KEY_FUNC
				KEYPLUS = [0x0f,0x80],//KEY_10PLUS
				KEYVOLUMEUP = [0x1a,0xff],//KeyVolumeUP
				KEYVOLUMEDOWM = [0x1b,0xff],//KeyVolumeDown
				KEYLIKE = [0x14,0xff],//KeyLike
				KEYMENU = [0x44,0xff],//KeyMenu
				KEYCHANNELUP = [0x4c,0xff],//KeyChannelUp
				KEYCHANNELDOWN = [0x4d,0xff],//KeyChannelDown
				KEYUP = [0x0a,0x95],//KeyUp
				KEYLEFT = [0x10,0x9b],//KeyLeft
				KEYRIGHT = [0x11,0x99],//KeyRight
				KEYDOWN = [0x0b,0x9a],//KeyDown
				KEYRET = [0x19,0x9e],//KEY_RET
				KEYESC = [0x18,0x96],//KEY_ESC
				KEYFUNC = [0x15,0x91],//KEY_FUNC
				KEYPREC = [0x12,0x90],//KEY_PREV
				KEYBACK = [0x13,0x98],//KEY_BACK
				KEYSLOW = [0x16,0x8c],//KEY_SLOW
				KEYPLAY = [0x17,0x82],//KEY_PLAY
				KEYFAST = [0x49,0x9c],//KEY_FAST
				KEYNEXT = [0x45,0x83],//KEY_NEXT
				KEYREC = [0x46,0x93],//KEY_REC
				KEYADDR = [0x47,0x97],//KEY_ADDR
				KEYEXITONE = [0x40,0xff],//KeyExt1
				KEYEXITTWO = [0x41,0xff],//KeyExt2
				KEYEXITTHREE = [0x42,0xff],//KeyExt3
				KEYEXITFOUR = [0x43,0xff],//KeyExt4
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

local cR8CA = R8CA();

return cR8CA;