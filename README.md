# WOLF4FMT
Wolfenstein 3d for Fujitsu FM TOWNS / Marty


使用方法：製品版「Wolfenstein 3D」のゲームフォルダ内にある.WL6(CONFIG.WL6除く)拡張子のファイルを「WOLF2MB.EXP」「WOLF4FMT.EXP」と同一ディレクトリにおいて「WOLF2MB.EXP」か「WOLF4FMT.EXP」を実行。
「WOLF2MB.EXP」はメモリが少ない機種向けなので通常は「WOLF4FMT.EXP」で実行してください。


～～～～～～～～～～～～～現バージョンV1.1 L20での仕様～～～～～～～～～～～～～

・ゲーム中ではESCキーと表記されているが、TOWNS版ではPAUSEキーが代わりに割り当てられている。

・キーボード操作でプレイするとランダムでクラッシュする(エミュレータ「津軽」で確認)。エミュレータ「うんづ」ではそもそもキーボード入力が受け付けない。コントローラでプレイする分には問題はない。

・フレームレートは上限60fps(オリジナルIBM-PC版は70fps)。

・YM3812用のデータを無理やりYM2612(FM TOWNSのFM音源)で鳴らしているためドラム音が鳴らない、鳴り方も変。


～～～～～～～～～～～～2MB機(マーティー含む)での起動方法～～～～～～～～～～～～

1.製品版Wolfenstein 3DにあるWL6拡張子のデータをCD-Rに書き込み

2.TownsOS V1.1 L30でフロッピーからの起動ディスク作成(初期化→CD演奏プログラム付)

3.起動ディスク内のRUN386.EXE、TBIOS.BIN、TBIOS.SYSをTownsOS V2.1 L20のものに上書き

4.起動ディスク直下にWOLF2MB.EXP、FDDフォルダ内のAUTOEXEC.BATとCONFIG.WL6の3ファイルをコピー
5.Wolfenstein 3Dのデータを書き込んだCDをセットして、起動ディスクからブート
