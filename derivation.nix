{ stdenv }:
stdenv.mkDerivation rec {
	name = "quest-${version}";
	version = "0.7";
	
	src = ./src2/.;

	nativeBuildInputs = [ ];
	buildInputs = [ ];

	buildPhase = ''
		gcc server.c -o quest-daemon
		gcc client.c -o quest-log
		gcc tui.c -o quest
	'';
	
	installPhase = ''
		mkdir -p $out/bin
		cp quest-daemon $out/bin
		cp quest-log $out/bin
		cp quest $out/bin
	'';
}
