/**
 * Cli.re
 *
 * Module for handling command-line arguments for Oni2
 */
open Rench;

type t = {
  folder: string,
  filesToOpen: list(string),
};

let create = (~folder, ~filesToOpen, ()) => {folder, filesToOpen};

let newline = "\n";

let show = (v: t) => {
  let files =
    List.fold_left((curr, p) => curr ++ newline ++ p, "", v.filesToOpen);

  "Folder: " ++ v.folder ++ newline ++ "Files: " ++ files;
};

let noop = () => ();

let parse = () => {
  let args: ref(list(string)) = ref([]);

  Arg.parse(
    [
      ("-f", Unit(Log.enablePrinting), ""),
      ("--nofork", Unit(Log.enablePrinting), ""),
      ("--checkhealth", Unit(HealthCheck.run), ""),
    ],
    arg => args := [arg, ...args^],
    "",
  );

  if (! Log.canPrint^) {
    /* On Windows, detach the application from the console if we're not logging to console */
    Utility.freeConsole();
  };

  let paths = args^ |> List.rev;
  let workingDirectory = Environment.getWorkingDirectory();

  let stripTrailingPathCharacter = s => {
    let len = String.length(s);
    if (len > 1 && s.[len - 1] == '/') {
      String.sub(s, 0, len - 1);
    } else {
      s;
    };
  };

  let isAbsolutePathWithTilde = s =>
    switch (s.[0]) {
    | '~' => !Sys.win32
    | _ => false
    | exception (Invalid_argument(_)) => false
    };

  let resolvePath = p => {
    let p =
      if (Path.isAbsolute(p) || isAbsolutePathWithTilde(p)) {
        p;
      } else {
        Path.join(workingDirectory, p);
      };

    p |> Path.normalize |> stripTrailingPathCharacter;
  };

  let absolutePaths = List.map(resolvePath, paths);

  let isDirectory = p =>
    switch (Sys.is_directory(p)) {
    | v => v
    | exception (Sys_error(_)) => false
    };

  let directories = List.filter(isDirectory, absolutePaths);
  let filesToOpen = List.filter(p => !isDirectory(p), absolutePaths);

  let homeOrWorkingDirectory =
    switch (Sys.getenv_opt("HOME")) {
    | Some(homePath) => homePath
    | None => workingDirectory
    };

  /* Set the folder to be opened, based on 4 options:
         - If a folder(s) is given, use the first.
         - If no folders are given, but files are, use the dir of the first file.
         - If no files or folders are given, and the path is, the root, "/", try and use the home directory
         - If none of the other conditions are met, use the working directory
     */

  let folder =
    switch (directories, filesToOpen, workingDirectory) {
    | ([hd, ..._], _, _) => hd
    | ([], [hd, ..._], _) => Rench.Path.dirname(hd)
    | ([], [], "/") => homeOrWorkingDirectory
    | ([], [], workingDirectory) => workingDirectory
    };

  {folder, filesToOpen};
};
