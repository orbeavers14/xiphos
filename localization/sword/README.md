# SWORD scripture-name catalogs

These files are copied verbatim from the official SWORD1.9.0 release.
Original translator credits remain in each file header.
Source: https://crosswire.org/ftpmirror/pub/sword/source/v1.9/sword-1.9.0.tar.gz
The release COPYING applies GNU GPL terms from LICENSE, retained here.
No Bible/module text is included.

| File | Original source | SHA256 |
|---|---|---|
| fr.conf | locales.d/fr-utf8.conf | 41bab359ad692cd742ad4c60d0a7f0fa15cad32722d45edb6b195880c3496e6f |
| de.conf | locales.d/de-utf8.conf | fd5f438f13bd3ed3615fba789b2c332259cc7529478ccb4f59c0b79b640a79bc |
| es.conf | locales.d/es-utf8.conf | 01abe59d24166a197dd4713609fc28e69ac0947441d20bc045f08f26fed60370 |

The core creates a private SWLocale for selector labels. It does not replace
the SDK global locale, mutate module keys, or localize persisted targets.
Typed localized-reference parsing is a separate compatibility concern.
