/* ../libAGPEngine.a */
 EXTERN(_ZN4Core17GetPluginRegisterEv);
 EXTERN(_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE);
 EXTERN(JNI_OnLoad);
 EXTERN(_ZN4Core10GetVersionEv);
 EXTERN(_ZN4Core12IsDebugBuildEv);
 EXTERN(_ZN4Core13GetVersionRevEv);
 EXTERN(_ZN4Core16GetVersionBranchEv);

VERSION {
	AGPEngineDLL_1.0 {
	global:
	/* ../libAGPEngine.a */
	_ZN4Core17GetPluginRegisterEv;
	_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE;
	JNI_OnLoad;
	_ZN4Core10GetVersionEv;
	_ZN4Core12IsDebugBuildEv;
	_ZN4Core13GetVersionRevEv;
	_ZN4Core16GetVersionBranchEv;
	local: *; /* hide everything */
	};
};