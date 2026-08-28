class TS8OB_7K4L extends PadBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "TS8OB_7K4L"
		
	// 指明是前面板协议还是遥控器协议，使用"PAD", "REMOTE"表示
	Type = "PAD"
	
	CommandLen = 4
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xaa]
	
	PadCodeFunc = {
			LEDLINKONE = [0x20, 0x00, 0x14, 0x00],//led: rec -> link
			LEDLINKTWO = [0x12, 0x00, 0x14, 0x00],//led: rec -> link
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

local cTS8OB_7K4L = TS8OB_7K4L();

return cTS8OB_7K4L;