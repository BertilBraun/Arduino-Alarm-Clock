<?php
	if ($_SERVER['REQUEST_METHOD'] === 'POST') {
		$Hour 		= $_POST['Hour'];
		$Minute 	= $_POST['Minute'];
		$SnoozeTime = $_POST['SnoozeTime'];
		$SoundPath 	= $_POST['SoundPath'];
		if (isset($_POST['DarkMode']) && $_POST['DarkMode'] == 'Yes') 
			$DarkMode 	= 1;
		else
			$DarkMode 	= 0;
		if (isset($_POST['ScreenOff']) && $_POST['ScreenOff'] == 'Yes') 
			$ScreenOff 	= 1;
		else
			$ScreenOff 	= 0;
		
		// SAVE
		$myfile = fopen('settings.txt', 'w');
		
		fwrite($myfile, 'Hour:' . $Hour . "\n");
		fwrite($myfile, 'Minute:' . $Minute . "\n");
		fwrite($myfile, 'SnoozeTime:' . $SnoozeTime . "\n");
		fwrite($myfile, 'SoundPath:' . $SoundPath . "\n");
		fwrite($myfile, 'DarkMode:' . $DarkMode . "\n");
		fwrite($myfile, 'ScreenOff:' . $ScreenOff);
		
		fclose($myfile);
	}
	else {
		// LOAD
		$myfile = fopen("settings.txt", "r") or die("Unable to open file!");

		$tokens 	= explode(":", fgets($myfile));
		$Hour 		= $tokens[1];
		$tokens 	= explode(":", fgets($myfile));
		$Minute 	= $tokens[1];
		$tokens 	= explode(":", fgets($myfile));
		$SnoozeTime = $tokens[1];
		$tokens 	= explode(":", fgets($myfile));
		$SoundPath 	= $tokens[1];
		$tokens 	= explode(":", fgets($myfile));
		$DarkMode 	= $tokens[1];
		$tokens 	= explode(":", fgets($myfile));
		$ScreenOff 	= $tokens[1];
		
		fclose($myfile);
	}
?>

<html>
<body>

	<h1>Update Settings</h1>

	<form action = "<?php $_PHP_SELF ?>" method = "POST">
		Hour: <input type="number" name="Hour" min="0" max="24" value="<?php echo $Hour; ?>">
		Minute: <input type="number" name="Minute" min="0" max="60" value="<?php echo $Minute; ?>">
		<br>
		SnoozeTime: <input type="number" name="SnoozeTime" min="0" max="20" value="<?php echo $SnoozeTime; ?>">
		<br>
		Path To Sound File: <input type="text" name="SoundPath" value="<?php echo $SoundPath; ?>">
		<br>
		DarkMode: <input type="checkbox" name="DarkMode" value="Yes" <?php if ($DarkMode == 1) echo "checked"; ?>>
		<br>
		ScreenOff: <input type="checkbox" name="ScreenOff" value="Yes" <?php if ($ScreenOff == 1) echo "checked"; ?>>
		<br>
		<br>
		<input type = "submit" />
	</form>
  
	<?php
		if ($_SERVER['REQUEST_METHOD'] === 'POST')
			echo "Upload Successful.";
	?>
  
</body>
</html>