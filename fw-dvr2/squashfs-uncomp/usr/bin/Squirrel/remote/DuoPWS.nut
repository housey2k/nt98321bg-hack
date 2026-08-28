class DuoPWS extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "DuoPWS"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEYONE = [0x07,0x87],
				KEYTWO = [0x05,0x86],
				KEYTHREE = [0x06,0x85],
				KEYFOUR = [0x0b,0x8b],
				KEYFIVE = [0x09,0x8a],
				KEYSIX = [0x0a,0x89],
				KEYSEVEN = [0x0f,0x8f],
				KEYEIGHT = [0x0d,0x8e],
				KEYNINE = [0x0e,0x8d],
				KEYZERO = [0x13,0x92],
				KEYSPLIT = [0x12,0x84],
				KEYSLOW = [0x17,0x8c],
				KEYPLAY = [0x15,0x82],
				KEYFAST = [0x16,0x9c],
				KEYREC = [0x18,0x93],
				KEYBACK = [0x14,0x98],
				KEYNEXT = [0x04,0x83],
				KEYLEFT = [0x1c,0x9b],
				KEYRIGHT = [0x00,0x99],
				KEYUP = [0x08,0x95],
				KEYDOWN = [0x10,0x9a],
				KEYENTER = [0x0c,0x9e],
				KEYMENU = [0x1b,0xa5],
				KEYADDR = [0x19,0x97],
				KEYESC = [0x1a,0x96],
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

local cDuoPWS = DuoPWS();

return cDuoPWS;