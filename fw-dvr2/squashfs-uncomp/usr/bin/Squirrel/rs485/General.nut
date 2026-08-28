class General extends RS485Base{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "General"
		
	// 指明是云台协议还是矩阵协议，使用"PTZ", "MATRIX"表示
	Type = "COMM"
	
	CommandLen = 11
	
	HeadLen = 1
	
	// 命令头数据
	HeadData = [0xF1]
	
	// 地址码
	RS232Addr = 0xFF
	
	// 是否有弹起消息
	UpMsg = 0
	
	// 是否通过脚本解析：0：通明串口 1：脚本解析 2：外部解析
	ParseMode = 2
	
}

local cGeneral = General();

return cGeneral;