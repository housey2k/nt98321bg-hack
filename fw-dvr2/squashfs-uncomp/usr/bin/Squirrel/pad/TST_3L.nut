class TST_3L extends PadBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "TST_3L"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "PAD"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]

	PadCodeFunc = {
			LEDNET = [0x4e, 0x00, 0x11, 0x00],//led: ready -> alarm
			INVALID = [0x11, 0x00, 0x16, 0x00],//led: alarm -> 电量检测 (对alarm消息不点亮任何灯)
			}
			
	function ParseData(cmdBuf)
	{
		foreach(k,v in PadCodeFunc)
		{
			local tmp = PadCodeFunc.rawget(k);
			if (cmdBuf[1] == tmp[0] && cmdBuf[2] == tmp[1])
			{
				local buf = [];
				buf.insert(0, cmdBuf[0]);
				buf.insert(1, tmp[2]);
				buf.insert(2, tmp[3]);
				buf.insert(3, cmdBuf[3]);
				return buf;
			}
		}
	}
}

local cTST_3L = TST_3L();

return cTST_3L;