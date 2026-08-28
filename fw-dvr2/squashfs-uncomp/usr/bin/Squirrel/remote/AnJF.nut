class AnJF extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "AnJF"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEYPOW = [0x88,0xff],//KEY_SEARCH->KEY_POWER
				KEYMENU = [0x90,0xa5],//KeyPre->KeyMenu
				KEYPTZ = [0x83,0xa4],//KeyNext->KeyPTZ
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

local cAnJF = AnJF();

return cAnJF;