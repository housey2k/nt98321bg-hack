class PelcoASCII extends PtzBase{
	// 协议的显示名称, 最好与文件名直接对应，不能超过16字符
	Name = "PELCOASCII"
		
	// 指明是云台协议还是矩阵协议，使用"PTZ", "MATRIX"表示
	Type = "MATRIX"
	
	// 以ms为单位
	Internal = 200		
	
	// 一下地址范围如无对应的地址范围，都设成0xFF
	
	// 云台地址范围
	CamAddrRange 		= [0, 9999]
	
	// 监视地址范围
	MonAddrRange		= [0, 9999]
	
	// 预置点范围
	PresetRange 		= [1, 9999]
	
	// 自动巡航线路范围
	TourRange			= [1, 255]
	
	// 轨迹线路范围
	PatternRange		= [1, 99]
	
	// 垂直速度范围
	TileSpeedRange 		= [1, 63]
	
	// 水平速度范围
	PanSpeedRange 		= [1, 64]
	
	// 辅助范围
	AuxRange 			= [1, 65536]
	
	AddrPos 			= 1
	PresetPos 		= 5
	TileSpeedPos 	= 5
	PanSpeedPos 	= 4
	AuxPos 				= 5
	
	cmd = []
	monitordata = []
	cameradata = []
	
	StartOpr = {
		TileUp 		= ['U', 'a'],
		TileDown 	= ['D', 'a'],
		PanLeft 	= ['L', 'a'],
		PanRight 	= ['R', 'a'],
		LeftUp 		= ['L', 'a', 'U', 'a'],
		LeftDown 	= ['L', 'a', 'D', 'a'],
		RightUp		= ['R', 'a', 'U', 'a'],
		RightDown = ['R', 'a', 'D', 'a'],
		
		ZoomWide 	= ['W', 'a'],
		ZoomTele 	= ['T', 'a'],
		FocusNear	= ['N', 'a'],
		FocusFar 	= ['F', 'a'],
		IrisSmall	= ['C', 'a'],
		IrisLarge	= ['O', 'a'],	
		
		SetPreset 	= ['^', 'a'],
		GoToPreset 	= ['\\', 'a'],
		
		SetPatternStart = ['/', 'a'],
		SetPatternStop 	= ['n', 'a'],
		StartPattern 		= ['p', 'a'],
    StopPattern     = ['s', 'a'],
    
    AuxOn 	= ['A', 'a'],
		AuxOff 	= ['B', 'a'],
		
		MatrixSwitch = ['M', 'a', '#', 'a']
	}
	
	StopOpr = {
		TileUp 		= ['s', 'a'],
		TileDown 	= ['s', 'a'],
		PanLeft 	= ['s', 'a'],
		PanRight 	= ['s', 'a'],
		LeftUp 		= ['s', 'a'],
		LeftDown 	= ['s', 'a'],
		RightUp		= ['s', 'a'],
		RightDown = ['s', 'a'],
		ZoomWide 	= ['s', 'a'],
		ZoomTele 	= ['s', 'a'],
		FocusNear = ['s', 'a'],
		FocusFar 	= ['s', 'a'],
		IrisSmall = ['s', 'a'],
		IrisLarge = ['s', 'a']
	}
	
	function conver(conArray, value)
	{
		local tmptable = [];
		local i = 0;
		local j = 0;
		local middletable = [];
	
		while (value > 0)
		{
			tmptable.insert(i, value % 10);
			value = (value - tmptable[i] ) / 10;
			i = i+1;		
		}
		
		local len = tmptable.len();
		for (j = 0; j < len; j++)
		{
			middletable.insert(j, tmptable[len - j - 1] + 0x30);
		}
		
		for (j = 0; j < conArray.len(); j++)
		{
			middletable.insert(len + j, conArray[j]);
		}
		
		return middletable;
	}

	function CheckSum()
	{
		local restr = [];
		local i = 0;
		local j = 0;
		
		for (j = 0; j < monitordata.len(); j++, i++)
		{
			restr.insert(i, monitordata[j]);
		}
		
		for (j = 0; j < cameradata.len(); j++, i++)
		{
			restr.insert(i, cameradata[j]);
		}
		
		if (cmd[0] != 'M')
		{
			for (j = 0; j < cmd.len(); j++, i++)
			{
				restr.insert(i, cmd[j]);
			}
		}
		cmd = restr;
	}
	
	function setCamAddr(addr)
	{
		local tmptable = ['#', 'a'];
		
		cameradata.clear();
	  cameradata = conver(tmptable, addr % 256);
	}
	
	function setMonAddr(addr)
  {
  	local tmptable = ['M', 'a'];
		
		monitordata.clear();
	  monitordata = conver(tmptable, addr % 256);
	}
	
	function setSpeed(ver, hor)
  {
	  if (cmd[0] != 's')
		{
			local retstr = [];
			local retstr1 = [];
			local retstr2 = [];
		
			if (cmd.len() > 2)
			{
				retstr1.insert(0, cmd[0]);
				retstr1.insert(1, cmd[1]);
				retstr2.insert(0, cmd[2]);
				retstr2.insert(1, cmd[3]);
				
				retstr1 = conver(retstr1, hor);
				local len = retstr1.len();
				local i = 0;
				for (i = 0; i < len; i++)
				{
					retstr.insert(i, retstr1[i]);
				}
				
				retstr2 = conver(retstr2, ver);
				for (i = 0; i < retstr2.len(); i++)
				{
					retstr.insert(len + i, retstr2[i]);
				}
			}	
			else
			{
				retstr1.insert(0, cmd[0]);
				retstr1.insert(1, cmd[1]);
				local value;
				if (ver != 0)
				{
					value = ver;
				}
				else
				{
					value = hor;
				}
				retstr = conver(retstr1, value);
			}
			cmd = retstr;
		}
  }
  
  function setPreset(preset)
	{
		cmd = conver(cmd, preset);
	}
	
	function setPattern(num)
	{
		cmd = conver(cmd, num);
	}
	
	function setAux(num)
	{
		cmd = conver(cmd, num);
	}
}
local cPelcoASCII = PelcoASCII();

return cPelcoASCII;