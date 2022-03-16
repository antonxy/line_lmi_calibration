let
	pkgs = import <nixpkgs> {};
	stdenv = pkgs.stdenv;
in pkgs.mkShell rec {
	name = "line-lmi-calibration";
	nativeBuildInputs = [ pkgs.meson pkgs.git pkgs.pkg-config pkgs.ninja];
	buildInputs = [
		(pkgs.opencv4.override {
			enableGtk3 = true;
		})
	];
}
