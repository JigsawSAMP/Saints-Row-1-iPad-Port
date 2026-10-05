# Removing Mac development files

The installed iPad app contains its native libraries, and the transferred game assets are in the iPad app's Documents/game folder. Deleting the source, generated code, extracted Mac asset copies, build directories or this checkout does not uninstall that app or remove its device saves/assets. The Mac is not used for streaming or gameplay.

**Signing is separate from storage.** Apple Personal Team provisioning profiles expire seven days after issuance. The installed app must then be rebuilt/re-signed and reinstalled; deleting local source does not stop that expiry. See [Apple's official Personal Team guidance](https://developer.apple.com/help/account/basics/about-your-developer-account). Paid-team signing follows the actual provisioning profile's validity; this repository does not promise a permanent install.

Before clearing the Mac, keep this source-only ZIP on GitHub or external storage. Keep your own compatible game disc/image separately: the public package contains no game content. For renewal on another Mac, restore the source, rebuild with your own account and use the same existing bundle ID/team without uninstalling the iPad app. Existing assets/saves normally remain with an in-place update; back up valuable saves before reinstalling.

For disposable outputs only:

```sh
python3 PortSupport/port.py clean
```

This removes PortBuild, generated guest code and extracted Mac asset copies. It does not remove the original ISO, source checkouts, Xcode account or iPad app. For a complete local checkout removal, preserve the verified source package and your game backup first, then delete the local checkout in Finder. Do not delete/offload the iPad app or its Documents container.
