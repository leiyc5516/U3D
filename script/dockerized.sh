#!/usr/bin/env bash
#
# Copyright (c) 2008-2022 the Urho3D project.
# Copyright (c) 2022-2024 the U3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

if [[ $# -eq 0 ]]; then echo "Usage: dockerized.sh linux|mingw|android|rpi|arm|web [command]"; exit 1; fi
if [[ $(id -u) -eq 0 ]]; then echo "Should not run using sudo or root user"; exit 1; fi

SCRIPT_DIR=$(cd "${0%/*}" || exit 1; pwd)
PROJECT_DIR=$(cd "${0%/*}/.." || exit 1; pwd)

# Determine which tool is available to use
if ! docker --version >/dev/null 2>&1; then
    if podman --version >/dev/null 2>&1; then
        use_podman=1
        # Disable SELinux protection in order to mount Urho3D project root directory from host to container in unprivileged mode
        run_option="--security-opt label=disable"
        # Podman mount volume as 'noexec' by default but we need 'exec' for Android build (aapt2 permission denied otherwise)
        mount_option=,exec
        # Podman by default pull from other OS registries before trying 'docker.io', so we need to be more explicit to avoid the retries
        registry=docker.io/
    else
        echo "Could not find Docker client or podman"
        exit 1
    fi
fi

d () {
    if [[ $use_podman ]]; then
        podman "$@"
    else
        docker "$@"
    fi
}

if [[ ! $DBE_NAME ]]; then
    DBE_NAME=${registry}urho3d/dockerized
fi

if [[ ! $DBE_TAG ]]; then
    # If the command failed or not on a tag then use 'master' by default
    DBE_TAG=$(git describe --exact-match 2>/dev/null || echo master)
fi

platform=$1; shift

# add new docker image with updated androidsdk
if [[ "$platform" == "android" ]]; then
    fishbone="U3D"
    if [[ $GITHUB_ACTIONS ]]; then
        dbe_image="okkoman/u3d-$platform:latest"
    else
        dbe_image="u3d-$platform"
    fi
# fallback to old docker images  
else
    fishbone="urho3d"
if [[ "$DBE_NAMING_SCHEME" == "tag" ]]; then
  dbe_image=$DBE_NAME:$DBE_TAG-$platform
else
  dbe_image=$DBE_NAME-$platform:$DBE_TAG
fi
fi

if [[ $DBE_REFRESH == 1 ]]; then
  d pull $dbe_image
fi
if [[ $GITHUB_ACTIONS ]]; then
  mkdir -p $GITHUB_WORKSPACE/build/cache
  mount_home_dir="--mount type=bind,source=$GITHUB_WORKSPACE/build/cache,target=/home/$fishbone$mount_option"
else
  mount_home_dir="--mount type=volume,source=$(id -u).urho3d_home_dir,target=/home/$fishbone$mount_option"
  interactive=-i
fi
if [[ $use_podman ]] || ( [[ $(d version -f '{{.Client.Version}}') =~ ^([0-9]+)\.0*([0-9]+)\. ]] && (( BASH_REMATCH[1] * 100 + BASH_REMATCH[2] >= 1809 )) ); then
    # podman or newer Docker client
    d run $interactive -t --rm -h fishtank $run_option \
        -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" -e PROJECT_DIR="$PROJECT_DIR" \
        --env-file "$SCRIPT_DIR/.env-file" \
        --mount type=bind,source="$PROJECT_DIR",target="$PROJECT_DIR" \
        $mount_home_dir \
        $dbe_image "$@"
else
    # Fallback workaround on older Docker CLI version
    d run $interactive -t --rm -h fishtank \
        -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" -e PROJECT_DIR="$PROJECT_DIR" \
        --env-file <(perl -ne 'chomp; print "$_\n" if defined $ENV{$_}' "$SCRIPT_DIR/.env-file") \
        --mount type=bind,source="$PROJECT_DIR",target="$PROJECT_DIR" \
        $mount_home_dir \
        $dbe_image "$@"
fi

# vi: set ts=4 sw=4 expandtab:
