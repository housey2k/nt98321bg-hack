class Weida extends RemoteBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "Weida"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "REMOTE"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
				KEY_SHUT = [0x84,0xa8],//
				KEY_HOME = [0x88,0xd6],//
				KEY_STOP = [0x81,0xa3],//
				KEY_VOLUME_DOWN = [0x93,0xd2],
				KEY_VOLUME_UP = [0x9d,0xd1],//
				KEY_SPLIT = [0x9f,0x84],
				KEY_MENU = [0x91,0xa5],//
				KEYPTZ = [0x94,0xa4],//KeyPtz
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

local cWeida = Weida();

return cWeida;