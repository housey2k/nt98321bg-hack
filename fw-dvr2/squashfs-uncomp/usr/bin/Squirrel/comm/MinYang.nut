class MinYang extends CommBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "MinYang"
		
	// 指明是云台协议还是矩阵协议，使用"PTZ", "MATRIX"表示
	Type = "COMM"
	
	CommandLen = 11
	
	// 是否通过脚本解析
	ParseMode = 1
	
	function ParseData(cmdBuf)
	{
		return " ";
	}
}

local cMinYang = MinYang();

return cMinYang;