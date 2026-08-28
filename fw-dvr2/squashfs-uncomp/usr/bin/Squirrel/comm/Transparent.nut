class Transparent extends CommBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "Transparent"
		
	// 指明是云台协议还是矩阵协议，使用"PTZ", "MATRIX"表示
	Type = "COMM"
	
	CommandLen = 0
		
	// 是否通过脚本解析：0：通明串口 1：脚本解析 2：外部解析
	ParseMode = 0
	
	function ParseData(cmdBuf)
	{
		return " ";
	}
}

local cTransparent = Transparent();

return cTransparent;