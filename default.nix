let
	pkgs = import <nixpkgs> {};
	stdenv = pkgs.stdenv;
in pkgs.llvmPackages_13.stdenv.mkDerivation rec {
	name = "line-lmi-calibration";
	nativeBuildInputs = [ pkgs.meson pkgs.git pkgs.pkg-config pkgs.ninja pkgs.lldb ];
	buildInputs = [
		(pkgs.opencv4.override {
			enableGtk3 = true;
		})
	];
}
