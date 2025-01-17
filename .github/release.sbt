/*
 * Copyright 2021 Concurrent Technologies Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import BuildSupport._
import play.api.libs.json._

lazy val packageData = Json
  .parse(scala.io.Source.fromFile("../../client/ts/package.json").mkString)
  .as[JsObject]
lazy val omegaVersion = packageData("version").as[String]

lazy val ghb_repo_owner = "ctc-oss"
lazy val ghb_repo = "omega-edit"
lazy val ghb_resolver = (
  s"GitHub ${ghb_repo_owner} Apache Maven Packages"
    at
      s"https://maven.pkg.github.com/${ghb_repo_owner}/${ghb_repo}"
)

// mostly used for getting all 3 jars working inside of one package
lazy val bashExtras = s"""declare new_classpath=\"$$app_classpath\"
declare windows_jar_file="com.ctc.omega-edit-native_2.13-${omegaVersion}-windows-64.jar"
declare linux_amd_jar_file="com.ctc.omega-edit-native_2.13-${omegaVersion}-linux-amd64.jar"
declare linux_aarch_jar_file="com.ctc.omega-edit-native_2.13-${omegaVersion}-linux-aarch64.jar"
declare macos_x86_jar_file="com.ctc.omega-edit-native_2.13-${omegaVersion}-macos-x86_64.jar"
declare macos_aarch_jar_file="com.ctc.omega-edit-native_2.13-${omegaVersion}-macos-aarch64.jar"
if [[ $$OSTYPE == "darwin"* ]]; then
  if [[ $$(uname -m) == "x86_64" ]]; then
    new_classpath=$$(echo $$new_classpath |\\
      sed -e "s/$${linux_aarch_jar_file}//" | \\
      sed -e "s/$${linux_amd_jar_file}//" | \\
      sed -e "s/$${windows_jar_file}//" | \\
      sed -e "s/$${macos_aarch_jar_file}//" \\
    )
  else
    new_classpath=$$(echo $$new_classpath |\\
      sed -e "s/$${linux_aarch_jar_file}//" | \\
      sed -e "s/$${linux_amd_jar_file}//" | \\
      sed -e "s/$${windows_jar_file}//" | \\
      sed -e "s/$${macos_x86_jar_file}//"\\
    )
  fi
else
  if [[ $$(uname -m) == "x86_64" ]]; then
    new_classpath=$$(echo $$new_classpath |\\
      sed -e "s/$${linux_aarch_jar_file}//" | \\
      sed -e "s/$${windows_jar_file}//" | \\
      sed -e "s/$${macos_aarch_jar_file}//" | \\
      sed -e "s/$${macos_x86_jar_file}//" \\
    )
  else
    new_classpath=$$(echo $$new_classpath |\\
      sed -e "s/$${linux_amd_jar_file}//" | \\
      sed -e "s/$${windows_jar_file}//" | \\
      sed -e "s/$${macos_x86_jar_file}//"\\
    )
  fi
fi"""

lazy val batchExtras = s"""
set "NEW_CLASSPATH=%APP_CLASSPATH%"
set "WINDOWS_JAR_FILE=com.ctc.omega-edit-native_2.13-${omegaVersion}-windows-64.jar"
set "NEW_CLASSPATH=%NEW_CLASSPATH:com.ctc.omega-edit-native_2.13-${omegaVersion}-linux-amd64.jar=!WINDOWS_JAR_FILE!%"
set "NEW_CLASSPATH=%NEW_CLASSPATH:com.ctc.omega-edit-native_2.13-${omegaVersion}-macos-x86_64.jar=!WINDOWS_JAR_FILE!%""""

lazy val commonSettings = {
  Seq(
    organization := "com.ctc",
    scalaVersion := "2.13.8",
    version := omegaVersion,
    licenses += ("Apache-2.0", new URL(
      "https://www.apache.org/licenses/LICENSE-2.0.txt"
    )),
    organizationName := "Concurrent Technologies Corporation",
    // git.useGitDescribe := true,
    // git.gitUncommittedChanges := false,
    licenses := Seq(("Apache-2.0", apacheLicenseUrl)),
    startYear := Some(2021),
    publishTo := Some(ghb_resolver),
    publishMavenStyle := true,
    credentials += Credentials(
      "GitHub Package Registry",
      "maven.pkg.github.com",
      ghb_repo_owner,
      System.getenv("GITHUB_TOKEN")
    )
  )
}

lazy val omega_edit = project
  .in(file("."))
  .settings(commonSettings)
  .settings(
    publish / skip := true
  )
  .aggregate(api, spi, native)

lazy val api = project
  .in(file("api"))
  .dependsOn(spi)
  .settings(commonSettings)
  .settings(
    name := "omega-edit",
    libraryDependencies ++= {
      Seq(
        "com.beachape" %% "enumeratum" % "1.7.0",
        "com.ctc" %% s"omega-edit-native" % version.value % Test classifier platform.id,
        "com.github.jnr" % "jnr-ffi" % "2.2.12",
        "org.scalatest" %% "scalatest" % "3.2.13" % Test
      )
    },
    scalacOptions ~= adjustScalacOptionsForScalatest,
    buildInfoKeys := Seq[BuildInfoKey](name, version, scalaVersion, sbtVersion),
    buildInfoObject := "ApiBuildInfo",
    buildInfoPackage := organization.value + ".omega_edit",
    // trim the dep to the native project from the pom
    pomPostProcess := filterScopedDependenciesFromPom,
    // ensure the native jar is published locally for tests
    resolvers += Resolver.mavenLocal,
    externalResolvers ++= Seq(
      ghb_resolver,
      Resolver.mavenLocal
    ),
    Compile / Keys.compile :=
      (Compile / Keys.compile)
        .dependsOn(native / publishM2)
        .value,
    Test / Keys.test :=
      (Test / Keys.test)
        .dependsOn(native / publishM2)
        .value
  )
  .enablePlugins(BuildInfoPlugin, GitVersioning)

lazy val native = project
  .in(file("native"))
  .dependsOn(spi)
  .settings(commonSettings)
  .settings(
    name := "omega-edit-native",
    artifactClassifier := Some(platform.id),
    Compile / packageBin / mappings += {
      baseDirectory
        .map(_ / s"$libdir/${mapping._1}")
        .value -> s"${version.value}/${mapping._2}"
    },
    Compile / packageDoc / publishArtifact := false,
    buildInfoKeys := Seq[BuildInfoKey](name, version, scalaVersion, sbtVersion),
    buildInfoPackage := organization.value + ".omega_edit.native",
    buildInfoKeys ++= Seq(
      "sharedLibraryName" -> mapping._1,
      "sharedLibraryOs" -> platform.os,
      "sharedLibraryArch" -> System.getProperty("os.arch"),
      "sharedLibraryPath" -> s"${version.value}/${mapping._2}"
    ),
    buildInfoOptions += BuildInfoOption.Traits(
      "com.ctc.omega_edit.spi.NativeBuildInfo"
    ),
    packagedArtifacts ++= Map(
      Artifact("omega-edit-native", "windows-64") -> file(
        s"../../../../omega-edit-native_${scalaBinaryVersion.value}-${version.value}-windows-64.jar"
      ),
      Artifact("omega-edit-native", "macos-x86_64") -> file(
        s"../../../../omega-edit-native_${scalaBinaryVersion.value}-${version.value}-macos-x86_64.jar"
      ),
      Artifact("omega-edit-native", "macos-aarch64") -> file(
        s"../../../../omega-edit-native_${scalaBinaryVersion.value}-${version.value}-macos-aarch64.jar"
      ),
      Artifact("omega-edit-native", "linux-aarch64") -> file(
        s"../../../../omega-edit-native_${scalaBinaryVersion.value}-${version.value}-linux-aarch64.jar"
      )
    )
  )
  .enablePlugins(BuildInfoPlugin, GitVersioning)

lazy val serv = project
  .in(file("serv"))
  .dependsOn(spi)
  .settings(commonSettings)
  .settings(
    name := "omega-edit-grpc-server",
    libraryDependencies ++= {
      Seq(
        "com.ctc" %% "omega-edit" % omegaVersion,
        "com.ctc" %% "omega-edit-native" % omegaVersion classifier s"linux-amd64",
        "com.ctc" %% "omega-edit-native" % omegaVersion classifier s"linux-aarch64",
        "com.ctc" %% "omega-edit-native" % omegaVersion classifier s"macos-x86_64",
        "com.ctc" %% "omega-edit-native" % omegaVersion classifier s"macos-aarch64",
        "com.ctc" %% "omega-edit-native" % omegaVersion classifier s"windows-64",
        "com.monovore" %% "decline" % "2.3.0",
        "org.scalatest" %% "scalatest" % "3.2.13" % Test
      )
    },
    excludeDependencies ++= Seq(
      ExclusionRule("org.checkerframework", "checker-compat-qual")
    ),
    scalacOptions ~= adjustScalacOptionsForScalatest,
    resolvers += Resolver.mavenLocal,
    externalResolvers ++= Seq(
      ghb_resolver,
      Resolver.mavenLocal
    ),
    Compile / PB.protoSources += baseDirectory.value / "../../../protos", // path relative to projects directory
    publishConfiguration := publishConfiguration.value.withOverwrite(true),
    publishLocalConfiguration := publishLocalConfiguration.value
      .withOverwrite(true),
    bashScriptExtraDefines += bashExtras,
    batScriptExtraDefines += batchExtras
  )
  .enablePlugins(
    AkkaGrpcPlugin,
    GitVersioning,
    JavaServerAppPackaging,
    UniversalPlugin
  )

lazy val spi = project
  .in(file("spi"))
  .settings(commonSettings)
  .settings(
    name := "omega-edit-spi"
  )
  .enablePlugins(GitVersioning)

addCommandAlias(
  "install",
  "; clean; native/publishM2; test; api/publishM2; spi/publishM2"
)
addCommandAlias("howMuchCoverage", "; clean; coverage; test; coverageAggregate")
addCommandAlias(
  "publishAll",
  "; clean; +native/publish; +api/publish; +spi/publish"
)
